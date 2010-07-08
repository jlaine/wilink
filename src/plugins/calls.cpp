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

#include <QMenu>

#include "calls.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "chat_roster.h"

CallWatcher::CallWatcher(Chat *chatWindow)
    : QObject(chatWindow)
{
    m_client = chatWindow->client();
    m_roster = chatWindow->rosterModel();
}

void CallWatcher::callContact()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();
    qDebug() << "call" << fullJid;
}

void CallWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    if (!m_client->isConnected())
        return;

    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    if (type == ChatRosterItem::Contact)
    {
        QStringList fullJids = m_roster->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
        if (fullJids.isEmpty())
            return;

        QAction *action = menu->addAction(tr("Call"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), this, SLOT(callContact()));
    }
}

// PLUGIN

class CallsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool CallsPlugin::initialize(Chat *chat)
{
    CallWatcher *watcher = new CallWatcher(chat);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            watcher, SLOT(rosterMenu(QMenu*, QModelIndex)));

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(calls, CallsPlugin)

