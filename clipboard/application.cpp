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

#include "application.h"
#include "clipboard.h"
#include "clipboardadaptor.h"
#include "historymodel.h"
#include "historywindow.h"

#include <QDBusConnection>

Application::Application(QObject *parent)
    : QObject(parent)
    , m_clipboard(new Clipboard(this))
    , m_window(new HistoryWindow(m_clipboard))
{
    new ClipboardAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Clipboard"), this);
}

bool Application::historyEnabled() const
{
    return m_clipboard->historyEnabled();
}

void Application::setHistoryEnabled(bool enabled)
{
    m_clipboard->setHistoryEnabled(enabled);
}

void Application::showHistory()
{
    m_window->open();
}

void Application::hideHistory()
{
    m_window->hide();
}

void Application::clearHistory()
{
    m_clipboard->history()->clear();
}
