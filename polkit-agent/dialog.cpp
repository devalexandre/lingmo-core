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

#include "dialog.h"

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>
#include <QDebug>
#include <QFile>

namespace {
const QUrl DialogUrl(QStringLiteral("qrc:/main.qml"));

QQmlEngine *sharedEngine()
{
    static QQmlEngine *engine = new QQmlEngine(QGuiApplication::instance());
    return engine;
}

QQmlComponent *dialogComponent()
{
    static QQmlComponent *component = new QQmlComponent(sharedEngine(), DialogUrl,
                                                        QQmlComponent::PreferSynchronous,
                                                        QGuiApplication::instance());
    return component;
}
}

void Dialog::preload()
{
    if (dialogComponent()->isError())
        qWarning() << dialogComponent()->errors();
}

Dialog::Dialog(const QString &action, const QString &message,
               const QString &cookie, const QString &identity,
               const QString &iconName,
               PolkitQt1::Agent::AsyncResult *result)
    : m_action(action)
    , m_message(message)
    , m_cookie(cookie)
    , m_identity(identity)
    , m_password(QString())
    , m_iconName(iconName)
    , m_result(result)
    , m_view(new QQuickView(sharedEngine(), nullptr))
{
    qDebug() << "Creating ConfirmationDialog";

    m_view->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_view->setResizeMode(QQuickView::SizeViewToRootObject);
    m_view->setDefaultAlphaBuffer(true);
    m_view->setColor(Qt::transparent);

    // This request's own context on the shared engine
    m_context = new QQmlContext(sharedEngine(), this);
    m_context->setContextProperty("confirmation", this);
    m_context->setContextProperty("rootWindow", m_view);
    QObject *root = dialogComponent()->create(m_context);
    if (!root)
        qWarning() << dialogComponent()->errors();
    m_view->setContent(DialogUrl, dialogComponent(), root);
    m_view->setVisible(false);
}

Dialog::~Dialog()
{
    qDebug() << "Deleting ConfirmationDialog";

    delete m_view;
}

void Dialog::setConfirmationResult(const QString &passwd)
{
    m_password = passwd;

    emit accepted();
}

void Dialog::rejected()
{
    emit cancel();
}

void Dialog::show()
{
    m_view->show();
    m_view->setScreen(qGuiApp->primaryScreen());
    m_view->setX((m_view->screen()->geometry().width() - m_view->geometry().width()) / 2);
    m_view->setY((m_view->screen()->geometry().height() - m_view->geometry().height()) / 2);
}

void Dialog::authenticationFailure()
{
    emit failure();
}

void Dialog::showMessage(const QString &text, bool error)
{
    emit message(text.trimmed(), error);
}

bool Dialog::fingerprint() const
{
    // Written by lingmo-settings' fingerprint-pam helper
    return QFile::exists(QStringLiteral("/etc/lingmo-fingerprint-enabled"));
}
