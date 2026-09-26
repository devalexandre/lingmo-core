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

#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QImage>
#include <QMap>
#include <QQuickImageProvider>

class QMimeData;
class QTimer;

struct ClipboardItem
{
    QString id;
    bool image = false;
    QString text;                      // the text, empty for images
    QImage picture;                    // the image, for image items
    QMap<QString, QByteArray> formats; // everything the app offered (kept in memory only)
    QDateTime time;
    QByteArray key;                    // what makes two items the same
};

// What was copied lately, newest first. Text and images are saved to
// ~/.local/share/lingmoos/clipboard so the history survives a restart.
class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IsImageRole,
        TextRole,
        ImageSizeRole,
        TimeRole,
    };

    static constexpr int MaxItems = 30;

    explicit HistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Records what is on the clipboard now (moved to the top when it's already there)
    void add(const QMimeData *mimeData);
    // Moves an item to the top, as it is on the clipboard again
    void promote(int row);

    const ClipboardItem &item(int row) const;
    QImage image(const QString &id) const;

    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    QString storageDir() const;
    void load();
    void scheduleSave();
    void save();
    void removeImageFile(const ClipboardItem &item);

    QList<ClipboardItem> m_items;
    QTimer *m_saveTimer;
    int m_nextId = 0;
};

// image://clipboard/<id>: the images in the history, scaled down for the list
class HistoryImageProvider : public QQuickImageProvider
{
public:
    explicit HistoryImageProvider(HistoryModel *model);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    HistoryModel *m_model;
};

#endif // HISTORYMODEL_H
