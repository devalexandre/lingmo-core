/*
 * Copyright (C) 2023-2024 LingmoOS Team.
 *
 * Author:     Kate Leet <kate@lingmoos.com>
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

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QFile>
#include <QLocale>
#include <QStandardPaths>
#include <QTranslator>

#include "application.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    QCommandLineOption historyOption(QStringLiteral("history"), QStringLiteral("Show the clipboard history"));
    parser.addOption(historyOption);
    parser.addHelpOption();
    parser.process(a);

    // One per session: a second start (Super+V runs "lingmo-clipboard --history")
    // just asks the running one
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("com.lingmo.Clipboard"))) {
        if (parser.isSet(historyOption)) {
            bus.call(QDBusMessage::createMethodCall(QStringLiteral("com.lingmo.Clipboard"), QStringLiteral("/Clipboard"),
                                                    QStringLiteral("com.lingmo.Clipboard"), QStringLiteral("showHistory")));
        }
        return 0;
    }

    const QString qmFilePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                      QStringLiteral("lingmo-clipboard/translations/%1.qm").arg(QLocale().name()));
    if (!qmFilePath.isEmpty()) {
        QTranslator *translator = new QTranslator(&a);
        if (translator->load(qmFilePath))
            a.installTranslator(translator);
        else
            delete translator;
    }

    Application application;
    if (parser.isSet(historyOption))
        application.showHistory();

    return a.exec();
}
