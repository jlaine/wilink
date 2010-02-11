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

#ifndef __WDESKTOP_CHAT_ROSTER_H__
#define __WDESKTOP_CHAT_ROSTER_H__

#include <QAbstractTableModel>
#include <QTableView>

#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppVCard.h"

class ChatRosterItem;
class QContextMenuEvent;
class QXmppClient;
class QXmppVCardManager;

class RosterModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    RosterModel(QXmppRoster *roster, QXmppVCardManager *vcard);
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QPixmap contactAvatar(const QString &bareJid) const;
    QString contactName(const QString &bareJid) const;

    void addPendingMessage(const QString &bareJid);
    void clearPendingMessages(const QString &bareJid);

public slots:
    void disconnected();

protected slots:
    void presenceChanged(const QString& bareJid, const QString& resource);
    void rosterChanged(const QString &jid);
    void rosterReceived();
    void vCardReceived(const QXmppVCard&);

private:
    QString contactStatus(const QString &bareJid) const;
    QPixmap contactStatusIcon(const QString &bareJid) const;

private:
    QXmppRoster *rosterManager;
    QXmppVCardManager *vcardManager;
    QMap<QString, QPixmap> rosterAvatars;
    QMap<QString, QString> rosterNames;
    QMap<QString, int> pendingMessages;

    ChatRosterItem *rootItem;
};

class RosterView : public QTableView
{
    Q_OBJECT

public:
    RosterView(RosterModel *model, QWidget *parent = NULL);
    void selectContact(const QString &jid);
    QSize sizeHint() const;

protected:
    void contextMenuEvent(QContextMenuEvent *event);

signals:
    void clicked(const QString &jid);
    void doubleClicked(const QString &jid);
    void removeContact(const QString &jid);

protected slots:
    void slotClicked();
    void slotDoubleClicked();
    void slotRemoveContact();

private:
    QMenu *contextMenu;
};

#endif
