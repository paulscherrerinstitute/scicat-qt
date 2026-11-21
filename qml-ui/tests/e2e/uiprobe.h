#pragma once

#include <QList>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QString>
#include <QStringList>

// What a test does to the window of the running application: finds items by objectName, and sends them the mouse,
// key and drag events a person would cause. The events stay inside this process.
//
// An item counts as shown when it is visible and not a row its list keeps for reuse. It is in view when its middle
// is inside the window and inside every item that clips or scrolls it.
class UiProbe
{
public:
    explicit UiProbe(QQuickWindow *window);

    QQuickWindow *window() const { return m_window; }

    // layouts and views update when a frame is prepared; this prepares one now
    void settle() const;

    // visible, and not a row its list keeps for reuse
    static bool shown(const QQuickItem *item);
    // every shown item of that name, top to bottom, then left to right; "within" narrows the search to one item
    QList<QQuickItem *> findAll(const QString &name, const QQuickItem *within = nullptr) const;
    // every shown item inside "within" (the whole window when null), in the same order
    QList<QQuickItem *> everything(const QQuickItem *within = nullptr) const;
    bool inView(const QQuickItem *item, QString *why = nullptr) const;
    // scrolls what scrolls above the item until its middle is in view; false when that does not get it there
    bool bringIntoView(const QQuickItem *item) const;
    // what is laid out beyond the edges of the window (at most four are named)
    QStringList cutOff() const;
    // its objectName, or its type and the named item it is in
    static QString nameOf(const QQuickItem *item);

    void click(const QQuickItem *item, bool twice = false) const;
    // "ctrl", "shift", "alt", "meta"; ctrl is the Command key on a Mac. False: not the name of a key.
    bool key(const QString &name, const QStringList &modifiers = {}) const;
    // to whatever has the keyboard focus
    void type(const QString &text) const;
    // clicks the field and replaces what is in it
    void typeInto(const QQuickItem *field, const QString &text) const;
    // files dragged in from a file manager
    void drop(const QQuickItem *item, const QStringList &files) const;

private:
    QPointer<QQuickWindow> m_window;
};
