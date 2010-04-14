/*
 * wiLink
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

#ifndef __WILINK_CHAT_ROSTER_H__
#define __WILINK_CHAT_ROSTER_H__

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QTableView>
#include <QTreeView>

#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppVCard.h"

#include "chat_roster_item.h"

class QContextMenuEvent;
class QXmppClient;
class QXmppVCardManager;

class ChatRosterPrompt : public QMessageBox
{
    Q_OBJECT

public:
    ChatRosterPrompt(QXmppClient *client, const QString &jid, QWidget *parent = 0);

private slots:
    void slotButtonClicked(QAbstractButton *button);

private:
    QXmppClient *m_client;
    QString m_jid;
};

class ChatRosterModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole,
        TypeRole,
        MessagesRole,
        AvatarRole,
        FlagsRole,
        UrlRole,
    };

    enum Feature {
        ChatStatesFeature = 1,
        FileTransferFeature = 2,
        VersionFeature = 4,
    };

    enum Flag {
        OptionsFlag = 1,
        MembersFlag = 2,
        KickFlag = 4,
    };

    ChatRosterModel(QXmppClient *client);
    ~ChatRosterModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex & index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QPixmap contactAvatar(const QString &bareJid) const;
    QStringList contactFeaturing(const QString &bareJid, ChatRosterModel::Feature) const;
    ChatRosterItem *contactItem(const QString &bareJid) const;
    QString contactName(const QString &bareJid) const;
    QString ownName() const;

    void addItem(ChatRosterItem::Type type, const QString &id, const QString &name = QString(), const QIcon &icon = QIcon());
    void removeItem(const QString &bareJid);

    void addPendingMessage(const QString &bareJid);
    void clearPendingMessages(const QString &bareJid);

public slots:
    void connected();
    void disconnected();

protected slots:
    void discoveryIqReceived(const QXmppDiscoveryIq &disco);
    void presenceChanged(const QString& bareJid, const QString& resource);
    void presenceReceived(const QXmppPresence &presence);
    void rosterChanged(const QString &jid);
    void rosterReceived();
    void vCardReceived(const QXmppVCard&);

private:
    QString contactStatus(const QString &bareJid) const;

private:
    QXmppClient *client;
    ChatRosterItem *rootItem;
    QString nickName;
    QMap<QString, int> clientFeatures;
};

class ChatRosterView : public QTreeView
{
    Q_OBJECT

public:
    enum Action
    {
        NoAction,
        AddAction,
        InviteAction,
        JoinAction,
        OptionsAction,
        MembersAction,
        RemoveAction,
        SendAction,
    };

    ChatRosterView(ChatRosterModel *model, QWidget *parent = NULL);
    void selectContact(const QString &jid);
    QSize sizeHint() const;

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void resizeEvent(QResizeEvent *e);

signals:
    void itemAction(int action, const QString &jid, int type);

protected slots:
    void selectionChanged(const QItemSelection & selected, const QItemSelection &deselected);
    void slotAction();
    void slotActivated(const QModelIndex &index);

private:
    ChatRosterModel *rosterModel;
};

#endif
