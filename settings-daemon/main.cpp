/*
 * Copyright (C) 2023-2024 LingmoOS Team.
 *
 * Author:     revenmartin <revenmartin@gmail.com>
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

#include <QSocketNotifier>

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

static int s_signalFd[2];

static void quitOnSignal(int)
{
    char c = 1;
    ssize_t n = ::write(s_signalFd[0], &c, sizeof(c));
    Q_UNUSED(n);
}

int main(int argc, char *argv[])
{
    Application a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // The session stops us with SIGTERM: quit cleanly so modules can undo
    // what they did to the X server (night light gamma ramps)
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signalFd) == 0) {
        QSocketNotifier *notifier = new QSocketNotifier(s_signalFd[1], QSocketNotifier::Read, &a);
        QObject::connect(notifier, &QSocketNotifier::activated, &a, &QCoreApplication::quit);

        struct sigaction action = {};
        action.sa_handler = quitOnSignal;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        sigaction(SIGTERM, &action, nullptr);
        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGHUP, &action, nullptr);
    }

    return a.exec();
}
