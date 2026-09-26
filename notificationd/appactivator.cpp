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

#include "appactivator.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>

#include <KWindowInfo>
#include <KX11Extras>

namespace AppActivator
{

static QString desktopFileFor(const QStringList &names)
{
    for (const QString &name : names) {
        const QString path = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, name + ".desktop");
        if (!path.isEmpty())
            return path;
    }
    return {};
}

bool activate(uint pid, const QStringList &names)
{
    QStringList wanted;
    for (const QString &name : names) {
        if (!name.isEmpty())
            // "org.kde.konsole" (reverse-DNS desktop entry) vs a "konsole" window class
            wanted << name.toLower() << QFileInfo(name).completeBaseName().toLower()
                   << name.section(QLatin1Char('.'), -1).toLower();
    }

    // Topmost matching window first
    const QList<WId> windows = KX11Extras::stackingOrder();
    for (auto it = windows.crbegin(); it != windows.crend(); ++it) {
        const KWindowInfo info(*it, NET::WMPid | NET::WMWindowType | NET::WMState, NET::WM2WindowClass);
        if (!info.valid() || info.hasState(NET::SkipTaskbar))
            continue;
        const NET::WindowType type = info.windowType(NET::NormalMask | NET::DialogMask);
        if (type != NET::Normal && type != NET::Dialog && type != NET::Unknown)
            continue;

        const bool samePid = pid > 0 && uint(info.pid()) == pid;
        const bool sameClass = wanted.contains(QString::fromUtf8(info.windowClassClass()).toLower())
                               || wanted.contains(QString::fromUtf8(info.windowClassName()).toLower());
        if (samePid || sameClass) {
            KX11Extras::forceActiveWindow(*it);
            return true;
        }
    }

    const QString desktopFile = desktopFileFor(names);
    if (!desktopFile.isEmpty())
        return QProcess::startDetached(QStringLiteral("gio"), {QStringLiteral("launch"), desktopFile});
    return false;
}

}
