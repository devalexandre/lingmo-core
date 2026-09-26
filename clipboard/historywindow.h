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

#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QQuickView>

class Clipboard;
class QSortFilterProxyModel;

// The Super+V popup: pick something copied earlier to paste it again
class HistoryWindow : public QQuickView
{
    Q_OBJECT

public:
    explicit HistoryWindow(Clipboard *clipboard);

    // Next to the pointer, on top of everything, with the keyboard
    void open();

    // Row of the filtered list the QML shows
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void setFilter(const QString &text);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QPoint cursorPosition() const;

signals:
    void opened();

protected:
    bool event(QEvent *event) override;

private:
    int sourceRow(int row) const;
    void pasteLater(int attempt = 0);

    Clipboard *m_clipboard;
    QSortFilterProxyModel *m_filter;
    QString m_targetClass; // window class of the app to paste into
};

#endif // HISTORYWINDOW_H
