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

#include "QXmppBookmarkManager.h"
#include "QXmppBookmarkSet.h"

#include "application.h"
#include "client.h"
#include "news.h"

class NewsListItem : public ChatModelItem
{
public:
    QString name;
    QUrl url;
};

NewsListModel::NewsListModel(QObject *parent)
    : ChatModel(parent)
{
    QHash<int, QByteArray> names;
    names.insert(AvatarRole, "avatar");
    names.insert(NameRole, "name");
    names.insert(UrlRole, "url");
    setRoleNames(names);

    NewsListItem *item = new NewsListItem;
    item->name = "BBC News - World";
    item->url = "http://feeds.bbci.co.uk/news/world/rss.xml";
    addItem(item, rootItem, 0);

    NewsListItem *item2 = new NewsListItem;
    item2->name = "BBC News - Technology";
    item2->url = "http://feeds.bbci.co.uk/news/technology/rss.xml";
    addItem(item2, rootItem, 1);
}

ChatClient *NewsListModel::client() const
{
    return m_client;
}

void NewsListModel::setClient(ChatClient *client)
{
    if (client != m_client) {

        m_client = client;

        if (m_client) {
            bool check;
            Q_UNUSED(check);

            // connect signals
            check = connect(client->bookmarkManager(), SIGNAL(bookmarksReceived(QXmppBookmarkSet)),
                            this, SLOT(_q_bookmarksReceived()));
            Q_ASSERT(check);

            if (client->bookmarkManager()->areBookmarksReceived())
                QMetaObject::invokeMethod(this, "_q_bookmarksReceived", Qt::QueuedConnection);
        }

        emit clientChanged(m_client);
    }
}

QVariant NewsListModel::data(const QModelIndex &index, int role) const
{
    NewsListItem *item = static_cast<NewsListItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AvatarRole) {
        return wApp->qmlUrl("rss.png");
    } else if (role == NameRole) {
        return item->name;
    } else if (role == UrlRole) {
        return item->url;
    }

    return QVariant();
}

void NewsListModel::_q_bookmarksReceived()
{
    Q_ASSERT(m_client);

    const QXmppBookmarkSet &bookmarks = m_client->bookmarkManager()->bookmarks();
    foreach (const QXmppBookmarkUrl &bookmark, bookmarks.urls()) {
    }
}


