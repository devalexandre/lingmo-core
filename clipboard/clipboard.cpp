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

#include "clipboard.h"
#include "historymodel.h"

#include <QApplication>
#include <QMimeData>
#include <QPixmap>
#include <QDebug>

#include <QBuffer>
#include <QSettings>

// Set by password managers (KeePassXC, ...) on secrets: never recorded
static const QString s_passwordManagerHint = QStringLiteral("x-kde-passwordManagerHint");

Clipboard::Clipboard(QObject *parent)
    : QObject(parent)
    , m_qtClipboard(qApp->clipboard())
    , m_history(new HistoryModel(this))
    , m_historyEnabled(QSettings(QStringLiteral("lingmoos"), QStringLiteral("clipboard")).value(QStringLiteral("History"), true).toBool())
{
    if (!m_historyEnabled)
        m_history->clear();

    connect(m_qtClipboard, &QClipboard::dataChanged, this, &Clipboard::onDataChanged);
}

HistoryModel *Clipboard::history() const
{
    return m_history;
}

bool Clipboard::historyEnabled() const
{
    return m_historyEnabled;
}

void Clipboard::setHistoryEnabled(bool enabled)
{
    if (enabled == m_historyEnabled)
        return;
    m_historyEnabled = enabled;
    QSettings(QStringLiteral("lingmoos"), QStringLiteral("clipboard")).setValue(QStringLiteral("History"), enabled);
    if (!enabled)
        m_history->clear();
    emit historyEnabledChanged();
}

void Clipboard::restore(int row)
{
    if (row < 0 || row >= m_history->rowCount())
        return;

    const ClipboardItem &item = m_history->item(row);
    QMimeData *mimeData = new QMimeData;
    for (auto it = item.formats.cbegin(); it != item.formats.cend(); ++it)
        mimeData->setData(it.key(), it.value());
    if (item.image)
        mimeData->setImageData(item.picture);
    else if (!mimeData->hasText())
        mimeData->setText(item.text);

    // Ours already: onDataChanged() leaves it alone
    mimeData->setData("application/x-lingmo-clipboard", QByteArray("1"));
    m_qtClipboard->setMimeData(mimeData);

    m_history->promote(row);
}

void Clipboard::onDataChanged()
{
    const QMimeData *mimeData = m_qtClipboard->mimeData();

    if (mimeData->formats().isEmpty())
        return;

    if (mimeData->hasFormat("application/x-lingmo-clipboard") &&
            mimeData->data("application/x-lingmo-clipboard") == "1")
        return;

    if (m_historyEnabled && mimeData->data(s_passwordManagerHint) != "secret")
        m_history->add(mimeData);

    QByteArray timeStamp = mimeData->data("TIMESTAMP");

    QMimeData *newMimeData = new QMimeData;
    if (mimeData->hasImage()) {
        QPixmap srcPix = m_qtClipboard->pixmap();

        QByteArray bArray;
        QBuffer buffer(&bArray);
        buffer.open(QIODevice::WriteOnly);

        srcPix.save(&buffer);

        newMimeData->setImageData(srcPix);
        newMimeData->setData("TIMESTAMP", timeStamp);
    }

    for (const QString &key : mimeData->formats()) {
        if (key == "image/png" || key == "application/x-qt-image")
            continue;

        newMimeData->setData(key, mimeData->data(key));
    }

    // lingmo flag.
    newMimeData->setData("application/x-lingmo-clipboard", QByteArray("1"));

    m_qtClipboard->setMimeData(newMimeData);
}
