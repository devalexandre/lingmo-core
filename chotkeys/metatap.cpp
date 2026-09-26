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

#include "metatap.h"

#include <QDebug>
#include <QSocketNotifier>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>

// Longer holds are Meta used as a modifier (or a change of mind), not a tap
static constexpr int kMaxTapMs = 600;

MetaTap::MetaTap(QObject *parent)
    : QObject(parent)
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display)
        return;

    int event, error;
    int major = 2, minor = 1;
    if (!XQueryExtension(m_display, "XInputExtension", &m_xiOpcode, &event, &error)
        || XIQueryVersion(m_display, &major, &minor) != Success) {
        qWarning() << "MetaTap: XInput 2 not available, Meta alone won't be a shortcut";
        XCloseDisplay(m_display);
        m_display = nullptr;
        return;
    }

    m_superLeft = XKeysymToKeycode(m_display, XK_Super_L);
    m_superRight = XKeysymToKeycode(m_display, XK_Super_R);

    unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {};
    XISetMask(mask, XI_RawKeyPress);
    XISetMask(mask, XI_RawKeyRelease);
    XISetMask(mask, XI_RawButtonPress);
    XIEventMask eventMask = {XIAllMasterDevices, sizeof(mask), mask};
    XISelectEvents(m_display, DefaultRootWindow(m_display), &eventMask, 1);
    XFlush(m_display);

    m_notifier = new QSocketNotifier(ConnectionNumber(m_display), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &MetaTap::processEvents);
}

MetaTap::~MetaTap()
{
    if (m_display)
        XCloseDisplay(m_display);
}

bool MetaTap::isValid() const
{
    return m_display != nullptr;
}

void MetaTap::processEvents()
{
    while (XPending(m_display)) {
        XEvent ev;
        XNextEvent(m_display, &ev);

        XGenericEventCookie *cookie = &ev.xcookie;
        if (cookie->type != GenericEvent || cookie->extension != m_xiOpcode
            || !XGetEventData(m_display, cookie))
            continue;

        const auto *raw = static_cast<XIRawEvent *>(cookie->data);
        const bool isSuper = raw->detail == m_superLeft || raw->detail == m_superRight;

        switch (cookie->evtype) {
        case XI_RawKeyPress:
            if (isSuper) {
                // Auto-repeat while held sends more presses: keep the first timestamp
                if (!m_armed) {
                    m_armed = true;
                    m_pressTime.start();
                }
            } else {
                m_armed = false;   // Meta+<key>: a shortcut, not a tap
            }
            break;
        case XI_RawKeyRelease:
            if (isSuper && m_armed) {
                m_armed = false;
                if (m_pressTime.elapsed() < kMaxTapMs)
                    emit tapped();
            }
            break;
        case XI_RawButtonPress:
            m_armed = false;       // Meta+click (window move/resize)
            break;
        }

        XFreeEventData(m_display, cookie);
    }
}
