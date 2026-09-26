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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class Clipboard;
class HistoryWindow;

// com.lingmo.Clipboard on the session bus (/Clipboard)
class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled)

public:
    explicit Application(QObject *parent = nullptr);

    bool historyEnabled() const;
    void setHistoryEnabled(bool enabled);

public slots:
    void showHistory();
    void hideHistory();
    void clearHistory();

private:
    Clipboard *m_clipboard;
    HistoryWindow *m_window;
};

#endif // APPLICATION_H
