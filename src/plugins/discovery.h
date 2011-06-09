/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#ifndef __WILINK_DISCOVERY_H__
#define __WILINK_DISCOVERY_H__

class QTimer;
class QXmppClient;
class QXmppDiscoveryManager;

#include "QXmppDiscoveryIq.h"

#include "model.h"

/** The DiscoveryModel class represents a set of service discovery results.
 */
class DiscoveryModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(bool details READ details WRITE setDetails NOTIFY detailsChanged)
    Q_PROPERTY(QXmppDiscoveryManager* manager READ manager WRITE setManager NOTIFY managerChanged)
    Q_PROPERTY(QString rootJid READ rootJid WRITE setRootJid NOTIFY rootJidChanged)
    Q_PROPERTY(QString rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)

public:
    DiscoveryModel(QObject *parent = 0);

    bool details() const;
    void setDetails(bool details);

    QXmppDiscoveryManager *manager() const;
    void setManager(QXmppDiscoveryManager *manager);

    QString rootJid() const;
    void setRootJid(const QString &rootJid);

    QString rootNode() const;
    void setRootNode(const QString &rootNode);

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void detailsChanged(bool details);
    void managerChanged(QXmppDiscoveryManager *manager);
    void rootJidChanged(const QString &rootJid);
    void rootNodeChanged(const QString &rootNode);

public slots:
    void refresh();

private slots:
    void infoReceived(const QXmppDiscoveryIq &disco);
    void itemsReceived(const QXmppDiscoveryIq &disco);

private:
    QXmppClient *m_client;
    bool m_details;
    QXmppDiscoveryManager *m_manager;
    QStringList m_requests;
    QString m_rootJid;
    QString m_rootNode;
    QTimer *m_timer;
};

#endif
