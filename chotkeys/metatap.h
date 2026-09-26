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

#ifndef METATAP_H
#define METATAP_H

#include <QElapsedTimer>
#include <QObject>

typedef struct _XDisplay Display;
class QSocketNotifier;

// Reports a tap of the Meta (Super) key on its own: pressed and released quickly
// with no other key or button in between. X can't grab a lone modifier without
// breaking Meta+<key> shortcuts, so this listens to XInput2 raw events instead,
// which every client receives regardless of grabs.
class MetaTap : public QObject
{
    Q_OBJECT

public:
    explicit MetaTap(QObject *parent = nullptr);
    ~MetaTap() override;

    bool isValid() const;

signals:
    void tapped();

private:
    void processEvents();

    Display *m_display = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    int m_xiOpcode = 0;
    int m_superLeft = 0;
    int m_superRight = 0;
    bool m_armed = false;
    QElapsedTimer m_pressTime;
};

#endif // METATAP_H
