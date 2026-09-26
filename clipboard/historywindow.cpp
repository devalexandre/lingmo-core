/*
 * Copyright (C) 2026 LingmoOS Team.
 *
 * Author:     devalexandre <alexandre@dev2learn.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "historywindow.h"
#include "clipboard.h"
#include "historymodel.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QTimer>

#include <functional>
#include <memory>

#include <KWindowInfo>
#include <KX11Extras>
#include <NETWM>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

// Terminals paste with Ctrl+Shift+V
static const QStringList s_terminals = {
    QStringLiteral("lingmo-terminal"), QStringLiteral("konsole"), QStringLiteral("xterm"),
    QStringLiteral("uxterm"), QStringLiteral("urxvt"), QStringLiteral("gnome-terminal-server"),
    QStringLiteral("gnome-terminal"), QStringLiteral("xfce4-terminal"), QStringLiteral("mate-terminal"),
    QStringLiteral("lxterminal"), QStringLiteral("qterminal"), QStringLiteral("terminator"),
    QStringLiteral("tilix"), QStringLiteral("alacritty"), QStringLiteral("kitty"),
    QStringLiteral("org.wezfurlong.wezterm"), QStringLiteral("st-256color"), QStringLiteral("guake"),
    QStringLiteral("yakuake"), QStringLiteral("cool-retro-term"), QStringLiteral("deepin-terminal"),
    QStringLiteral("com.mitchellh.ghostty"), QStringLiteral("foot"),
};

HistoryWindow::HistoryWindow(Clipboard *clipboard)
    : QQuickView()
    , m_clipboard(clipboard)
    , m_filter(new QSortFilterProxyModel(this))
{
    m_filter->setSourceModel(clipboard->history());
    m_filter->setFilterRole(HistoryModel::TextRole);
    m_filter->setFilterCaseSensitivity(Qt::CaseInsensitive);

    // Override-redirect: the app we paste into keeps the focus meanwhile
    setFlags(Qt::Popup);
    setResizeMode(QQuickView::SizeViewToRootObject);
    setColor(Qt::transparent);

    engine()->addImageProvider(QStringLiteral("clipboard"), new HistoryImageProvider(clipboard->history()));
    rootContext()->setContextProperty(QStringLiteral("historyWindow"), this);
    rootContext()->setContextProperty(QStringLiteral("historyModel"), m_filter);
    rootContext()->setContextProperty(QStringLiteral("clipboard"), clipboard);
    setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    setVisible(false);
}

void HistoryWindow::open()
{
    // Where the paste goes: the window that is active before we show up
    m_targetClass.clear();
    if (const WId active = KX11Extras::activeWindow())
        m_targetClass = QString::fromUtf8(KWindowInfo(active, NET::Properties(), NET::WM2WindowClass).windowClassClass()).toLower();

    setFilter(QString());
    emit opened();

    // Next to the pointer, inside the screen it is on
    const QPoint cursor = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(cursor);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect area = screen->availableGeometry();
    const int margin = 12;
    int x = cursor.x() - width() / 2;
    int y = cursor.y() + margin;
    if (y + height() > area.bottom() - margin)
        y = cursor.y() - height() - margin;
    x = qBound(area.left() + margin, x, qMax(area.left() + margin, area.right() - width() - margin));
    y = qBound(area.top() + margin, y, qMax(area.top() + margin, area.bottom() - height() - margin));
    setPosition(x, y);

    show();
    raise();
    requestActivate();

    // Super+V is still held by lingmo-chotkeys' grab for a moment: try again until ours works
    auto grab = std::make_shared<std::function<void(int)>>();
    *grab = [this, grab](int attempt) {
        if (!isVisible())
            return;
        const bool keyboard = setKeyboardGrabEnabled(true);
        const bool mouse = setMouseGrabEnabled(true);
        if ((!keyboard || !mouse) && attempt < 40)
            QTimer::singleShot(25, this, [grab, attempt] { (*grab)(attempt + 1); });
    };
    (*grab)(0);
}

int HistoryWindow::sourceRow(int row) const
{
    return m_filter->mapToSource(m_filter->index(row, 0)).row();
}

void HistoryWindow::activate(int row)
{
    const int source = sourceRow(row);
    if (source < 0)
        return;

    hide();
    m_clipboard->restore(source);

    if (QSettings(QStringLiteral("lingmoos"), QStringLiteral("clipboard")).value(QStringLiteral("PasteOnSelect"), true).toBool())
        QTimer::singleShot(100, this, [this] { pasteLater(); });
}

void HistoryWindow::remove(int row)
{
    const int source = sourceRow(row);
    if (source >= 0)
        m_clipboard->history()->remove(source);
}

QPoint HistoryWindow::cursorPosition() const
{
    return QCursor::pos();
}

void HistoryWindow::clear()
{
    m_clipboard->history()->clear();
}

void HistoryWindow::setFilter(const QString &text)
{
    m_filter->setFilterFixedString(text);
}

void HistoryWindow::pasteLater(int attempt)
{
    // Keys still held (Super from Super+V, Shift, ...) would turn Ctrl+V into something else
    if (QGuiApplication::queryKeyboardModifiers() != Qt::NoModifier) {
        if (attempt < 100)
            QTimer::singleShot(20, this, [this, attempt] { pasteLater(attempt + 1); });
        return;
    }

    auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    Display *display = x11 ? x11->display() : nullptr;
    int event, error, major, minor;
    if (!display || !XTestQueryExtension(display, &event, &error, &major, &minor))
        return;

    const bool terminal = s_terminals.contains(m_targetClass);
    const KeyCode control = XKeysymToKeycode(display, XK_Control_L);
    const KeyCode shift = XKeysymToKeycode(display, XK_Shift_L);
    const KeyCode v = XKeysymToKeycode(display, XK_v);

    XTestFakeKeyEvent(display, control, True, CurrentTime);
    if (terminal)
        XTestFakeKeyEvent(display, shift, True, CurrentTime);
    XTestFakeKeyEvent(display, v, True, CurrentTime);
    XTestFakeKeyEvent(display, v, False, CurrentTime);
    if (terminal)
        XTestFakeKeyEvent(display, shift, False, CurrentTime);
    XTestFakeKeyEvent(display, control, False, CurrentTime);
    XFlush(display);
}

bool HistoryWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        // A click anywhere else closes it
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (!QRect(QPoint(0, 0), size()).contains(mouseEvent->position().toPoint())) {
            hide();
            return true;
        }
        break;
    }
    case QEvent::Show:
        KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager | NET::SkipSwitcher);
        break;
    case QEvent::Hide:
        setKeyboardGrabEnabled(false);
        setMouseGrabEnabled(false);
        break;
    default:
        break;
    }
    return QQuickView::event(event);
}
