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

#include "historymodel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

// Formats bigger than this aren't kept for pasting back (the text or image still is)
static constexpr qsizetype MaxFormatsSize = 16 * 1024 * 1024;

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_saveTimer(new QTimer(this))
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500);
    connect(m_saveTimer, &QTimer::timeout, this, &HistoryModel::save);

    load();
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return QVariant();

    const ClipboardItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return item.id;
    case IsImageRole:
        return item.image;
    case TextRole:
    case Qt::DisplayRole:
        // Enough for the list (and its search): a whole book stays out of the delegates
        return item.text.left(2000);
    case ImageSizeRole:
        return item.picture.size();
    case TimeRole:
        return item.time;
    }
    return QVariant();
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {IdRole, "itemId"},
        {IsImageRole, "isImage"},
        {TextRole, "text"},
        {ImageSizeRole, "imageSize"},
        {TimeRole, "time"},
    };
}

void HistoryModel::add(const QMimeData *mimeData)
{
    ClipboardItem item;
    item.time = QDateTime::currentDateTime();

    if (mimeData->hasImage()) {
        item.image = true;
        item.picture = qvariant_cast<QImage>(mimeData->imageData());
        if (item.picture.isNull())
            return;
        item.picture = item.picture.convertToFormat(QImage::Format_ARGB32);
        QCryptographicHash hash(QCryptographicHash::Md5);
        hash.addData(QByteArrayView(reinterpret_cast<const char *>(item.picture.constBits()), item.picture.sizeInBytes()));
        hash.addData(QByteArray::number(item.picture.width()));
        item.key = "i:" + hash.result().toHex();
    } else if (mimeData->hasText()) {
        item.text = mimeData->text();
        if (item.text.trimmed().isEmpty())
            return;
        item.key = "t:" + QCryptographicHash::hash(item.text.toUtf8(), QCryptographicHash::Sha1).toHex();
    } else {
        return;
    }

    qsizetype size = 0;
    for (const QString &format : mimeData->formats()) {
        // Images are pasted back from the QImage; Qt's own formats are rebuilt from it
        if (item.image && (format.startsWith(QLatin1String("image/")) || format == QLatin1String("application/x-qt-image")))
            continue;
        const QByteArray data = mimeData->data(format);
        size += data.size();
        item.formats.insert(format, data);
    }
    if (size > MaxFormatsSize)
        item.formats.clear();

    // Same thing copied again: move it to the top
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).key == item.key) {
            item.id = m_items.at(i).id;
            m_items[i] = item;
            promote(i);
            dataChanged(index(0), index(0));
            scheduleSave();
            return;
        }
    }

    item.id = QString::number(m_nextId++);

    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();

    while (m_items.size() > MaxItems)
        remove(m_items.size() - 1);

    emit countChanged();
    scheduleSave();
}

void HistoryModel::promote(int row)
{
    if (row <= 0 || row >= m_items.size())
        return;
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0);
    m_items.move(row, 0);
    endMoveRows();
    scheduleSave();
}

const ClipboardItem &HistoryModel::item(int row) const
{
    return m_items.at(row);
}

QImage HistoryModel::image(const QString &id) const
{
    for (const ClipboardItem &item : m_items) {
        if (item.id == id)
            return item.picture;
    }
    return QImage();
}

void HistoryModel::remove(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    const ClipboardItem item = m_items.takeAt(row);
    endRemoveRows();
    removeImageFile(item);
    emit countChanged();
    scheduleSave();
}

void HistoryModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    for (const ClipboardItem &item : std::as_const(m_items))
        removeImageFile(item);
    m_items.clear();
    endResetModel();
    emit countChanged();
    scheduleSave();
}

QString HistoryModel::storageDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/lingmoos/clipboard");
}

void HistoryModel::load()
{
    QFile file(storageDir() + QStringLiteral("/history.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonArray entries = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        ClipboardItem item;
        item.time = QDateTime::fromString(entry.value(QStringLiteral("time")).toString(), Qt::ISODate);
        if (entry.value(QStringLiteral("type")).toString() == QLatin1String("image")) {
            const QString name = entry.value(QStringLiteral("file")).toString();
            item.image = true;
            item.picture.load(storageDir() + '/' + name, "PNG");
            if (item.picture.isNull())
                continue;
            item.key = "i:" + QFileInfo(name).completeBaseName().toLatin1();
        } else {
            item.text = entry.value(QStringLiteral("text")).toString();
            if (item.text.isEmpty())
                continue;
            item.key = "t:" + QCryptographicHash::hash(item.text.toUtf8(), QCryptographicHash::Sha1).toHex();
        }
        item.id = QString::number(m_nextId++);
        m_items << item;
        if (m_items.size() == MaxItems)
            break;
    }
}

void HistoryModel::scheduleSave()
{
    m_saveTimer->start();
}

void HistoryModel::save()
{
    // Copied text can be private: only this user may read it
    QDir().mkpath(storageDir());
    QFile::setPermissions(storageDir(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    QJsonArray entries;
    for (const ClipboardItem &item : std::as_const(m_items)) {
        QJsonObject entry;
        entry.insert(QStringLiteral("time"), item.time.toString(Qt::ISODate));
        if (item.image) {
            const QString name = QString::fromLatin1(item.key.mid(2)) + QStringLiteral(".png");
            const QString path = storageDir() + '/' + name;
            if (!QFile::exists(path) && item.picture.save(path, "PNG"))
                QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            entry.insert(QStringLiteral("type"), QStringLiteral("image"));
            entry.insert(QStringLiteral("file"), name);
        } else {
            entry.insert(QStringLiteral("type"), QStringLiteral("text"));
            entry.insert(QStringLiteral("text"), item.text);
        }
        entries << entry;
    }

    QSaveFile file(storageDir() + QStringLiteral("/history.json"));
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    file.write(QJsonDocument(entries).toJson(QJsonDocument::Compact));
    file.commit();
}

void HistoryModel::removeImageFile(const ClipboardItem &item)
{
    if (item.image)
        QFile::remove(storageDir() + '/' + QString::fromLatin1(item.key.mid(2)) + QStringLiteral(".png"));
}

HistoryImageProvider::HistoryImageProvider(HistoryModel *model)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_model(model)
{
}

QImage HistoryImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image = m_model->image(id);
    if (size)
        *size = image.size();
    if (!image.isNull() && requestedSize.isValid()
            && (image.width() > requestedSize.width() || image.height() > requestedSize.height()))
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}
