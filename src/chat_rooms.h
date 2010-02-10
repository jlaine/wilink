/*
 * wDesktop
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WDESKTOP_CHAT_ROOMS_H__
#define __WDESKTOP_CHAT_ROOMS_H__

#include <QAbstractItemModel>
#include <QTableView>
#include <QTreeView>

class QXmppClient;
class QXmppDiscoveryIq;
class QXmppPresence;

class ChatRoomsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ChatRoomsModel(QXmppClient *client);

    void addRoom(const QString &bareJid);
    QString roomName(const QString &bareJid) const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex & index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void presenceReceived(const QXmppPresence &presence);

private:
    QXmppClient *xmppClient;
    QStringList roomKeys;
    QMap<QString, QStringList> roomParticipants;
};

class ChatRoomsView : public QTreeView
{
    Q_OBJECT

public:
    ChatRoomsView(ChatRoomsModel *model, QWidget *parent = NULL);
    QSize sizeHint() const;

protected slots:
    void slotClicked();
    void slotDoubleClicked();

signals:
    void clicked(const QString &jid);
    void doubleClicked(const QString &jid);
};

#endif
