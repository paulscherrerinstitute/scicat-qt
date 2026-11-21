#include "uidriver.h"

#include "backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QMetaMethod>
#include <QStyleHints>
#include <QTest>
#include <QTimer>
#include <QUrl>
#include <memory>

UiDriver::UiDriver(QQuickWindow *window, Backend *backend, const QString &scriptFile, const QString &outputDir, QObject *parent)
    : QObject(parent)
    , m_window(window)
    , m_ui(window)
    , m_backend(backend)
    , m_outputDir(outputDir)
    , m_log(outputDir + QStringLiteral("/log.txt"))
{
    QDir().mkpath(outputDir);
    if (!m_log.open(QIODevice::WriteOnly | QIODevice::Text))
        qWarning("cannot write %s", qPrintable(m_log.fileName()));
    QFile script(scriptFile);
    QJsonParseError error;
    if (script.open(QIODevice::ReadOnly))
        m_steps = QJsonDocument::fromJson(script.readAll(), &error).array();
    if (m_steps.isEmpty())
        fail(QStringLiteral("no steps in %1 (%2)").arg(scriptFile, error.errorString()));
}

void UiDriver::start()
{
    m_clock.start();
    QTimer::singleShot(300, this, &UiDriver::next);
}

void UiDriver::next()
{
    if (m_index >= m_steps.size() || !m_window) {
        log(QStringLiteral("done: %1 steps, %2 screenshots, %3 failures, %4 s").arg(m_index).arg(m_shots).arg(m_failures).arg(m_clock.elapsed() / 1000.0, 0, 'f', 1));
        m_log.close();
        emit finished(m_failures);
        return;
    }
    const QJsonObject step = m_steps.at(m_index++).toObject();
    if (step.contains(QLatin1String("waitFor"))) {
        waitFor(step.value(QLatin1String("waitFor")).toObject());
        return;
    }
    if (step.contains(QLatin1String("check")) || step.contains(QLatin1String("exists")) || step.contains(QLatin1String("absent"))) {
        expect(step);
        return;
    }
    run(step);
    QTimer::singleShot(step.contains(QLatin1String("wait")) ? step.value(QLatin1String("wait")).toInt() : 60, this, &UiDriver::next);
}

void UiDriver::run(const QJsonObject &step)
{
    if (step.contains(QLatin1String("note"))) {
        log(QStringLiteral("-- ") + step.value(QLatin1String("note")).toString());
    } else if (step.contains(QLatin1String("wait"))) {
        // handled by the delay before the next step
    } else if (step.contains(QLatin1String("click")) || step.contains(QLatin1String("doubleClick"))) {
        const bool twice = step.contains(QLatin1String("doubleClick"));
        const QJsonValue target = step.value(twice ? QLatin1String("doubleClick") : QLatin1String("click"));
        if (QQuickItem *item = findInView(target)) {
            m_ui.click(item, twice);
            log(QStringLiteral("click %1").arg(describe(target)));
        }
    } else if (step.contains(QLatin1String("type"))) {
        if (step.contains(QLatin1String("into"))) {
            QQuickItem *item = findInView(step.value(QLatin1String("into")));
            if (!item)
                return;
            m_ui.click(item, false);
#ifdef Q_OS_MACOS
            key(QStringLiteral("A"), {QStringLiteral("meta")});
#else
            key(QStringLiteral("A"), {QStringLiteral("ctrl")});
#endif
            key(QStringLiteral("Backspace"), {});
        }
        settle(); // a frame passes between the click and the typing, as it does for a person
        m_ui.type(step.value(QLatin1String("type")).toString());
        log(QStringLiteral("type %1 characters").arg(step.value(QLatin1String("type")).toString().size()));
    } else if (step.contains(QLatin1String("key"))) {
        settle();
        key(step.value(QLatin1String("key")).toString(), step.value(QLatin1String("modifiers")).toArray());
        log(QStringLiteral("key %1").arg(step.value(QLatin1String("key")).toString()));
    } else if (step.contains(QLatin1String("drop"))) {
        const QJsonObject spec = step.value(QLatin1String("drop")).toObject();
        if (QQuickItem *item = findInView(spec.value(QLatin1String("on"))))
            drop(item, spec.value(QLatin1String("files")).toArray());
    } else if (step.contains(QLatin1String("shot"))) {
        QCoreApplication::processEvents();
        const QString file = step.value(QLatin1String("shot")).toString();
        const QImage image = grab();
        if (image.isNull() || !image.save(m_outputDir + QLatin1Char('/') + file))
            fail(QStringLiteral("could not save %1").arg(file));
        else
            log(QStringLiteral("shot %1 (%2x%3, title \"%4\")").arg(file).arg(image.width()).arg(image.height()).arg(m_window->title()));
        ++m_shots;
        const QStringList beyond = m_ui.cutOff();
        if (!beyond.isEmpty())
            fail(QStringLiteral("in %1 these reach beyond the window (%2 x %3): %4").arg(file).arg(m_window->width()).arg(m_window->height()).arg(beyond.join(QStringLiteral("; "))));
    } else if (step.contains(QLatin1String("resize"))) {
        const QJsonArray size = step.value(QLatin1String("resize")).toArray();
        m_window->resize(size.at(0).toInt(), size.at(1).toInt());
        settle();
        const QQuickItem *content = m_window->contentItem();
        if (QSizeF(content->width(), content->height()).toSize() != m_window->size())
            fail(QStringLiteral("the window is %1 x %2 but its content is %3 x %4").arg(m_window->width()).arg(m_window->height()).arg(content->width()).arg(content->height()));
    } else if (step.contains(QLatin1String("scheme"))) {
        const bool dark = step.value(QLatin1String("scheme")).toString() == QLatin1String("dark");
        QGuiApplication::styleHints()->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
        settle();
    } else if (step.contains(QLatin1String("set"))) {
        const QJsonObject spec = step.value(QLatin1String("set")).toObject();
        if (QQuickItem *item = find(spec.value(QLatin1String("target"))))
            item->setProperty(spec.value(QLatin1String("property")).toString().toUtf8().constData(), spec.value(QLatin1String("value")).toVariant());
    } else if (step.contains(QLatin1String("scroll"))) {
        const QJsonObject spec = step.value(QLatin1String("scroll")).toObject();
        if (QQuickItem *item = find(spec.value(QLatin1String("target")))) {
            QQuickItem *flickable = item->property("contentY").isValid() ? item : item->property("contentItem").value<QQuickItem *>();
            if (flickable && flickable->property("contentY").isValid() && flickable->property("contentHeight").toReal() <= 0) {
                fail(QStringLiteral("%1 has no content height, so it cannot scroll").arg(describe(spec.value(QLatin1String("target")))));
            } else if (flickable && flickable->property("contentY").isValid()) {
                // "end" scrolls to the bottom
                const qreal end = qMax(0.0, flickable->property("contentHeight").toReal() - flickable->height());
                flickable->setProperty("contentY", spec.value(QLatin1String("y")).isString() ? end : qMin(end, spec.value(QLatin1String("y")).toDouble()));
            } else
                fail(QStringLiteral("%1 cannot be scrolled").arg(describe(spec.value(QLatin1String("target")))));
        }
    } else if (step.contains(QLatin1String("tabWalk"))) {
        // press Tab and write down where keyboard focus lands each time
        QStringList stops;
        const QString start = UiProbe::nameOf(m_window->activeFocusItem());
        for (int i = 0; i < step.value(QLatin1String("tabWalk")).toInt(); ++i) {
            QTest::keyClick(m_window, Qt::Key_Tab);
            QCoreApplication::processEvents();
            QQuickItem *focused = m_window->activeFocusItem();
            stops << UiProbe::nameOf(focused);
            settle();
            QString why;
            if (focused && !m_ui.inView(focused, &why))
                fail(QStringLiteral("after %1 x Tab the focus is on %2, which is not in view: %3").arg(i + 1).arg(stops.constLast(), why));
        }
        log(QStringLiteral("tab order from %1: %2").arg(start, stops.join(QStringLiteral(" > "))));
        const QString first = step.value(QLatin1String("first")).toString();
        if (!first.isEmpty() && stops.value(0) != first)
            fail(QStringLiteral("the first Tab should reach %1 but reached %2").arg(first, stops.value(0)));
    } else if (step.contains(QLatin1String("tabTo"))) {
        // press Tab until focus is on the named item or inside it (the current row of a list, the field of a combo box)
        const QString name = step.value(QLatin1String("tabTo")).toString();
        if (!find(name))
            return;
        for (int presses = 1; presses <= 80; ++presses) {
            QTest::keyClick(m_window, Qt::Key_Tab);
            QCoreApplication::processEvents();
            settle();
            QQuickItem *focused = m_window->activeFocusItem();
            bool reached = false;
            for (QQuickItem *item = focused; item && !reached; item = item->parentItem())
                reached = item->objectName() == name;
            if (!reached)
                continue;
            QString why;
            if (!m_ui.inView(focused, &why))
                fail(QStringLiteral("Tab reached %1, but it is not in view: %2").arg(name, why));
            else if (focused->property("visualFocus").isValid() && !focused->property("visualFocus").toBool())
                fail(QStringLiteral("Tab reached %1, but nothing shows that it has the focus").arg(name));
            else
                log(QStringLiteral("Tab x %1 to %2").arg(presses).arg(name));
            return;
        }
        fail(QStringLiteral("80 x Tab never reached %1; focus is on %2").arg(name, UiProbe::nameOf(m_window->activeFocusItem())));
    } else if (step.contains(QLatin1String("closeWindow"))) {
        // what the window's close button does; the application may refuse and ask first
        m_window->close();
        QCoreApplication::processEvents();
        log(m_window->isVisible() || !m_window->property("closingConfirmed").toBool() ? QStringLiteral("close asked, window stays")
                                                                                     : QStringLiteral("close asked, window closed"));
    } else if (step.contains(QLatin1String("invoke"))) {
        invoke(step.value(QLatin1String("invoke")).toObject());
    } else {
        fail(QStringLiteral("unknown step %1").arg(QString::fromUtf8(QJsonDocument(step).toJson(QJsonDocument::Compact))));
    }
}

void UiDriver::settle()
{
    m_ui.settle();
}

QImage UiDriver::grab()
{
    return m_window->grabWindow();
}

QString UiDriver::describe(const QJsonValue &target)
{
    return target.isObject() ? QString::fromUtf8(QJsonDocument(target.toObject()).toJson(QJsonDocument::Compact)) : target.toString();
}

void UiDriver::log(const QString &line)
{
    m_log.write(line.toUtf8() + '\n');
    m_log.flush();
}

void UiDriver::fail(const QString &line)
{
    ++m_failures;
    log(QStringLiteral("FAIL step %1: %2").arg(m_index).arg(line));
}

QQuickItem *UiDriver::find(const QJsonValue &target, bool report)
{
    if (report)
        settle();
    const QJsonObject spec = target.isObject() ? target.toObject() : QJsonObject{{QStringLiteral("name"), target.toString()}};
    const QString name = spec.value(QLatin1String("name")).toString();
    QList<QQuickItem *> candidates = m_ui.findAll(name);
    if (spec.contains(QLatin1String("text"))) {
        // by what it says, for items that have no name of their own, such as the entries of a combo box
        QList<QQuickItem *> saying;
        for (QQuickItem *item : std::as_const(candidates)) {
            if (item->property("text").toString() == spec.value(QLatin1String("text")).toString())
                saying.append(item);
        }
        candidates = saying;
    }
    if (spec.contains(QLatin1String("where"))) {
        const QJsonObject where = spec.value(QLatin1String("where")).toObject();
        QList<QQuickItem *> kept;
        for (QQuickItem *item : std::as_const(candidates)) {
            QString unused;
            if (matches(item->property(where.value(QLatin1String("property")).toString().toUtf8().constData()), where, &unused))
                kept.append(item);
        }
        candidates = kept;
    }
    const int nth = spec.value(QLatin1String("nth")).toInt(0);
    if (nth < candidates.size())
        return candidates.at(nth);
    if (report)
        fail(QStringLiteral("nothing visible matches %1").arg(describe(target)));
    return nullptr;
}

QQuickItem *UiDriver::findInView(const QJsonValue &target)
{
    QQuickItem *item = find(target);
    QString why;
    if (item && !m_ui.inView(item, &why)) {
        fail(QStringLiteral("%1 is not in view: %2").arg(describe(target), why));
        return nullptr;
    }
    return item;
}

bool UiDriver::matches(const QVariant &actual, const QJsonObject &spec, QString *expected)
{
    if (spec.contains(QLatin1String("equals"))) {
        const QVariant wanted = spec.value(QLatin1String("equals")).toVariant();
        *expected = QStringLiteral("\"%1\"").arg(wanted.toString());
        return actual.toString() == wanted.toString();
    }
    if (spec.contains(QLatin1String("contains"))) {
        const QString wanted = spec.value(QLatin1String("contains")).toString();
        *expected = QStringLiteral("something containing \"%1\"").arg(wanted);
        return actual.toString().contains(wanted);
    }
    if (spec.contains(QLatin1String("atLeast"))) {
        *expected = QStringLiteral("at least %1").arg(spec.value(QLatin1String("atLeast")).toDouble());
        return actual.toDouble() >= spec.value(QLatin1String("atLeast")).toDouble();
    }
    *expected = QStringLiteral("anything");
    return actual.isValid();
}

void UiDriver::key(const QString &name, const QJsonArray &modifiers)
{
    QStringList held;
    for (const QJsonValue &modifier : modifiers)
        held << modifier.toString();
    if (!m_ui.key(name, held))
        fail(QStringLiteral("\"%1\" is not the name of a key (Qt's names: PgDown, PgUp, Return, Escape, ...)").arg(name));
}

void UiDriver::drop(QQuickItem *item, const QJsonArray &files)
{
    QStringList paths;
    for (const QJsonValue &file : files)
        paths << file.toString();
    m_ui.drop(item, paths);
    log(QStringLiteral("drop %1 files").arg(paths.size()));
}

void UiDriver::invoke(const QJsonObject &spec)
{
    QObject *object = m_backend;
    QString where = spec.value(QLatin1String("object")).toString();
    if (spec.contains(QLatin1String("target"))) {
        object = find(spec.value(QLatin1String("target")));
        where = describe(spec.value(QLatin1String("target")));
        if (!object)
            return;
    } else {
        const QStringList path = where.split(QLatin1Char('.'));
        for (int i = 1; i < path.size() && object; ++i)
            object = object->property(path.at(i).toUtf8().constData()).value<QObject *>();
    }
    const QByteArray method = spec.value(QLatin1String("method")).toString().toUtf8();
    const QJsonArray arguments = spec.value(QLatin1String("args")).toArray();
    if (!object) {
        fail(QStringLiteral("no object %1").arg(where));
        return;
    }
    const QMetaObject *meta = object->metaObject();
    for (int i = 0; i < meta->methodCount(); ++i) {
        const QMetaMethod candidate = meta->method(i);
        if (candidate.name() != method || candidate.parameterCount() != arguments.size())
            continue;
        QVariantList values;
        for (int a = 0; a < arguments.size(); ++a) {
            QVariant value = arguments.at(a).toVariant();
            if (candidate.parameterMetaType(a) == QMetaType::fromType<QUrl>())
                value = QUrl(value.toString());
            else
                value.convert(candidate.parameterMetaType(a));
            values << value;
        }
        const bool ok = arguments.isEmpty() ? candidate.invoke(object)
                        : arguments.size() == 1 ? candidate.invoke(object, QGenericArgument(values[0].typeName(), values[0].constData()))
                        : candidate.invoke(object, QGenericArgument(values[0].typeName(), values[0].constData()),
                                           QGenericArgument(values[1].typeName(), values[1].constData()));
        if (ok)
            log(QStringLiteral("invoke %1.%2").arg(where, QString::fromUtf8(method)));
        else
            fail(QStringLiteral("could not invoke %1").arg(QString::fromUtf8(method)));
        return;
    }
    fail(QStringLiteral("%1 has no method %2 with %3 arguments").arg(where, QString::fromUtf8(method)).arg(arguments.size()));
}

// Tries "attempt" now and then every "interval" ms until it says it is done. Its argument says whether this is the
// last try, on which it has to report what is wrong. The next step follows when it is done.
void UiDriver::poll(int timeout, int interval, const std::function<bool(bool)> &attempt)
{
    auto deadline = std::make_shared<QElapsedTimer>();
    deadline->start();
    auto *timer = new QTimer(this);
    timer->setInterval(interval);
    const auto tryNow = [this, timeout, deadline, timer, attempt] {
        settle();
        if (!attempt(deadline->elapsed() > timeout))
            return false;
        timer->stop();
        timer->deleteLater();
        QTimer::singleShot(60, this, &UiDriver::next);
        return true;
    };
    connect(timer, &QTimer::timeout, this, tryNow);
    if (!tryNow())
        timer->start();
}

void UiDriver::waitFor(const QJsonObject &spec)
{
    const int timeout = spec.value(QLatin1String("timeout")).toInt(8000);
    auto waited = std::make_shared<QElapsedTimer>();
    waited->start();
    poll(timeout, 100, [this, spec, timeout, waited](bool last) {
        QQuickItem *item = find(spec.value(QLatin1String("target")), false);
        QString expected;
        const QString what = QString::fromUtf8(QJsonDocument(spec).toJson(QJsonDocument::Compact));
        if (item && (!spec.contains(QLatin1String("property"))
                     || matches(item->property(spec.value(QLatin1String("property")).toString().toUtf8().constData()), spec, &expected))) {
            log(QStringLiteral("waited %1 ms for %2").arg(waited->elapsed()).arg(what));
            return true;
        }
        if (last)
            fail(QStringLiteral("gave up after %1 ms waiting for %2").arg(timeout).arg(what));
        return last;
    });
}

// "check", "exists" and "absent" say what a person sees a moment after acting. One that does not hold at once is
// tried again for a second and a half: a search that waits for the typing to pause, or a list that takes a change in
// with the next frame, is not a failure. One that holds at once passes at once.
void UiDriver::expect(const QJsonObject &step)
{
    poll(1500, 50, [this, step](bool last) {
        if (step.contains(QLatin1String("check"))) {
            const QJsonObject spec = step.value(QLatin1String("check")).toObject();
            QQuickItem *item = find(spec.value(QLatin1String("target")), last);
            if (!item)
                return last;
            const QVariant actual = item->property(spec.value(QLatin1String("property")).toString().toUtf8().constData());
            QString expected;
            const QString what = QStringLiteral("%1.%2").arg(describe(spec.value(QLatin1String("target"))), spec.value(QLatin1String("property")).toString());
            if (matches(actual, spec, &expected)) {
                log(QStringLiteral("PASS %1 %2 (is \"%3\")").arg(what, expected, actual.toString()));
                return true;
            }
            if (last)
                fail(QStringLiteral("%1 should be %2 but is \"%3\"").arg(what, expected, actual.toString()));
            return last;
        }
        if (step.contains(QLatin1String("exists"))) {
            const QJsonValue target = step.value(QLatin1String("exists"));
            QQuickItem *item = find(target, last);
            QString why;
            if (item && m_ui.inView(item, &why)) {
                log(QStringLiteral("PASS shown: %1").arg(describe(target)));
                return true;
            }
            if (item && last)
                fail(QStringLiteral("%1 is not in view: %2").arg(describe(target), why));
            return last;
        }
        const QJsonValue target = step.value(QLatin1String("absent"));
        if (!find(target, false)) {
            log(QStringLiteral("PASS not shown: %1").arg(describe(target)));
            return true;
        }
        if (last)
            fail(QStringLiteral("%1 is shown but should not be").arg(describe(target)));
        return last;
    });
}
