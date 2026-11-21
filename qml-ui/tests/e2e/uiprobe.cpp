#include "uiprobe.h"

#include <QCoreApplication>
#include <QDropEvent>
#include <QKeySequence>
#include <QMimeData>
#include <QRegularExpression>
#include <QTest>
#include <QUrl>
#include <QtQuick/private/qquickitem_p.h>
#include <algorithm>
#include <type_traits>

UiProbe::UiProbe(QQuickWindow *window)
    : m_window(window)
{
}

void UiProbe::settle() const
{
    QCoreApplication::processEvents();
    if (m_window)
        m_window->grabWindow();
    QCoreApplication::processEvents();
}

// culled: a row a list keeps for reuse, or one outside its viewport
bool UiProbe::shown(const QQuickItem *item)
{
    return item->isVisible() && !QQuickItemPrivate::get(item)->culled;
}

// "all": every shown item; else those of that name
static QList<QQuickItem *> collect(QQuickItem *from, const QString &name, bool all)
{
    QList<QQuickItem *> found;
    QList<QQuickItem *> stack{from};
    while (!stack.isEmpty()) {
        QQuickItem *item = stack.takeLast();
        if (!UiProbe::shown(item))
            continue;
        if (all || item->objectName() == name)
            found.append(item);
        const QList<QQuickItem *> children = item->childItems();
        for (auto it = children.crbegin(); it != children.crend(); ++it)
            stack.append(*it);
    }
    // top to bottom, then left to right, so a number follows what a reader sees
    std::stable_sort(found.begin(), found.end(), [](QQuickItem *a, QQuickItem *b) {
        const QPointF pa = a->mapToScene(QPointF(0, 0));
        const QPointF pb = b->mapToScene(QPointF(0, 0));
        return pa.y() != pb.y() ? pa.y() < pb.y() : pa.x() < pb.x();
    });
    return found;
}

QList<QQuickItem *> UiProbe::findAll(const QString &name, const QQuickItem *within) const
{
    if (!m_window)
        return {};
    return collect(within ? const_cast<QQuickItem *>(within) : m_window->contentItem(), name, false);
}

QList<QQuickItem *> UiProbe::everything(const QQuickItem *within) const
{
    if (!m_window)
        return {};
    return collect(within ? const_cast<QQuickItem *>(within) : m_window->contentItem(), QString(), true);
}

bool UiProbe::inView(const QQuickItem *item, QString *why) const
{
    const QPointF middle = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const auto outside = [&](const QRectF &area, const QString &of) {
        if (area.contains(middle))
            return false;
        if (why)
            *why = QStringLiteral("its middle (%1, %2) is outside %3 (%4, %5 %6 x %7)").arg(middle.x(), 0, 'f', 0).arg(middle.y(), 0, 'f', 0)
                       .arg(of).arg(area.x(), 0, 'f', 0).arg(area.y(), 0, 'f', 0).arg(area.width(), 0, 'f', 0).arg(area.height(), 0, 'f', 0);
        return true;
    };
    if (outside(QRectF(0, 0, m_window->width(), m_window->height()), QStringLiteral("the window")))
        return false;
    for (QQuickItem *above = item->parentItem(); above; above = above->parentItem()) {
        if ((above->clip() || above->inherits("QQuickFlickable"))
            && outside(above->mapRectToScene(QRectF(0, 0, above->width(), above->height())), nameOf(above)))
            return false;
    }
    return true;
}

bool UiProbe::bringIntoView(const QQuickItem *item) const
{
    if (inView(item))
        return true;
    for (QQuickItem *above = item->parentItem(); above; above = above->parentItem()) {
        if (!above->inherits("QQuickFlickable"))
            continue;
        const QQuickItem *content = above->property("contentItem").value<QQuickItem *>();
        if (!content)
            continue;
        const qreal middle = item->mapToItem(content, QPointF(item->width() / 2, item->height() / 2)).y();
        const qreal first = above->property("originY").toReal();
        const qreal last = qMax(first, first + above->property("contentHeight").toReal() - above->height());
        above->setProperty("contentY", qBound(first, middle - above->height() / 2, last));
        settle();
    }
    return inView(item);
}

// A shadow is drawn larger than what casts it, and may reach beyond the window. Nobody reads it or clicks it.
static bool isShadow(const QQuickItem *item)
{
    return item->inherits("QQuickShaderEffectSource") || item->inherits("QQuickShaderEffect")
           || (item->property("elevation").isValid() && item->property("source").isValid()); // the Material style's elevation effect
}

// Content of a scrolling or clipping item may be larger than it.
QStringList UiProbe::cutOff() const
{
    QStringList found;
    const QRectF window(-2, -2, m_window->width() + 4, m_window->height() + 4);
    QList<QQuickItem *> stack{m_window->contentItem()};
    while (!stack.isEmpty() && found.size() < 4) {
        QQuickItem *item = stack.takeLast();
        if (!shown(item) || qFuzzyIsNull(item->opacity()) || isShadow(item))
            continue;
        const QRectF area = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
        if (!area.isEmpty() && !window.contains(area)) {
            found << QStringLiteral("%1 (%2, %3 %4 x %5)").arg(nameOf(item)).arg(area.x(), 0, 'f', 0).arg(area.y(), 0, 'f', 0).arg(area.width(), 0, 'f', 0).arg(area.height(), 0, 'f', 0);
            continue; // what is inside it follows from it
        }
        if (!item->clip() && !item->inherits("QQuickFlickable"))
            stack.append(item->childItems());
    }
    return found;
}

QString UiProbe::nameOf(const QQuickItem *item)
{
    if (!item)
        return QStringLiteral("nothing");
    if (!item->objectName().isEmpty())
        return item->objectName();
    QString type = QString::fromLatin1(item->metaObject()->className());
    type.remove(QRegularExpression(QStringLiteral("_QMLTYPE_\\d+$|^QQuick")));
    for (QQuickItem *above = item->parentItem(); above; above = above->parentItem()) {
        if (!above->objectName().isEmpty())
            return QStringLiteral("%1 in %2").arg(type, above->objectName());
    }
    return type;
}

void UiProbe::click(const QQuickItem *item, bool twice) const
{
    const QPointF scene = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const QPointF global = m_window->mapToGlobal(scene);
    const auto send = [&](QEvent::Type type, Qt::MouseButtons buttons) {
        QMouseEvent event(type, scene, scene, global, Qt::LeftButton, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(m_window, &event);
    };
    QMouseEvent move(QEvent::MouseMove, scene, scene, global, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &move);
    send(QEvent::MouseButtonPress, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, Qt::NoButton);
    if (twice) {
        send(QEvent::MouseButtonDblClick, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, Qt::NoButton);
    }
    QCoreApplication::processEvents();
}

bool UiProbe::key(const QString &name, const QStringList &modifiers) const
{
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    for (const QString &m : modifiers) {
        mods |= m == QLatin1String("ctrl") ? Qt::ControlModifier : m == QLatin1String("shift") ? Qt::ShiftModifier
              : m == QLatin1String("alt") ? Qt::AltModifier : m == QLatin1String("meta") ? Qt::ControlModifier : Qt::NoModifier;
    }
#ifdef Q_OS_MACOS
    // Qt maps the Command key to Qt::ControlModifier and Control to Qt::MetaModifier
    if (modifiers.contains(QStringLiteral("ctrl")) && !modifiers.contains(QStringLiteral("meta")))
        mods = (mods & ~Qt::ControlModifier) | Qt::MetaModifier;
#endif
    const QKeySequence sequence(name);
    const Qt::Key code = sequence.isEmpty() ? Qt::Key_unknown : sequence[0].key();
    if (code == Qt::Key_unknown)
        return false;
    // through QTest, so that shortcuts are matched first, as for a real key press
    QTest::keyClick(m_window, code, mods);
    QCoreApplication::processEvents();
    return true;
}

void UiProbe::type(const QString &text) const
{
    for (const QChar character : text) {
        const QKeySequence sequence{QString(character)};
        const int code = sequence.isEmpty() ? 0 : sequence[0].key();
        QKeyEvent press(QEvent::KeyPress, code, Qt::NoModifier, QString(character));
        QCoreApplication::sendEvent(m_window, &press);
        QKeyEvent release(QEvent::KeyRelease, code, Qt::NoModifier, QString(character));
        QCoreApplication::sendEvent(m_window, &release);
    }
    QCoreApplication::processEvents();
}

void UiProbe::typeInto(const QQuickItem *field, const QString &text) const
{
    click(field);
    // "select all" is Command+A on a Mac and Ctrl+A elsewhere; Qt calls both the Control modifier
    key(QStringLiteral("A"), {QStringLiteral("meta")});
    key(QStringLiteral("Backspace"));
    settle(); // a frame passes between the click and the typing, as it does for a person
    type(text);
}

// The enter and move events of a drag take a position in whole pixels in older Qt versions and one in fractions of
// a pixel in newer ones, where the old form is on its way out.
template <typename Event>
static Event dragEvent(QPointF position, const QMimeData *mime)
{
    if constexpr (std::is_constructible_v<Event, QPointF, Qt::DropActions, const QMimeData *, Qt::MouseButtons, Qt::KeyboardModifiers>)
        return Event(position, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    else
        return Event(position.toPoint(), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
}

void UiProbe::drop(const QQuickItem *item, const QStringList &files) const
{
    QMimeData mime;
    QList<QUrl> urls;
    for (const QString &file : files)
        urls << QUrl::fromLocalFile(file);
    mime.setUrls(urls);
    const QPointF position = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
    auto enter = dragEvent<QDragEnterEvent>(position, &mime);
    QCoreApplication::sendEvent(m_window, &enter);
    auto move = dragEvent<QDragMoveEvent>(position, &mime);
    QCoreApplication::sendEvent(m_window, &move);
    QDropEvent release(position, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &release);
    QCoreApplication::processEvents();
}
