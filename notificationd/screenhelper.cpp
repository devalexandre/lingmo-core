/*
 * Copyright (C) 2023-2024 LingmoOS Team.
 *
 * Author:     Reion Wong <reion@lingmoos.com>
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

#include "screenhelper.h"
#include <QApplication>
#include <QScreen>

ScreenHelper::ScreenHelper(QObject *parent)
    : QObject(parent)
{
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &ScreenHelper::screenGeometryChanged);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &ScreenHelper::availableScreenRectChanged);

    // Track the geometry of whichever screen is primary now
    auto follow = [this](QScreen *screen) {
        for (QScreen *s : qApp->screens())
            disconnect(s, nullptr, this, nullptr);
        connect(screen, &QScreen::geometryChanged, this, &ScreenHelper::screenGeometryChanged);
        connect(screen, &QScreen::availableGeometryChanged, this, &ScreenHelper::availableScreenRectChanged);
    };
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, follow);
    follow(qApp->primaryScreen());
}

QRect ScreenHelper::screenGeometry() const
{
    return qApp->primaryScreen()->geometry();
}

QRect ScreenHelper::availableScreenRect() const
{
    return qApp->primaryScreen()->availableGeometry();
}
