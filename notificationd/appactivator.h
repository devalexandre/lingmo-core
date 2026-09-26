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

#ifndef APPACTIVATOR_H
#define APPACTIVATOR_H

#include <QString>

// Brings the app behind a notification to the front when the notification has no
// "default" action to invoke: raises its window (by PID, else by window class) or,
// when it has none, launches its .desktop entry.
namespace AppActivator
{
bool activate(uint pid, const QStringList &names);
}

#endif // APPACTIVATOR_H
