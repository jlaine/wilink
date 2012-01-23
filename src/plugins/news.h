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

#ifndef __WILINK_NEWS_H__
#define __WILINK_NEWS_H__

#include <QUrl>

#include "model.h"

class ChatClient;
class QModelIndex;

class NewsListModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(ChatClient* client READ client WRITE setClient NOTIFY clientChanged)

public:
    NewsListModel(QObject *parent = 0);

    ChatClient *client() const;
    void setClient(ChatClient *client);

    QVariant data(const QModelIndex &index, int role) const;

signals:
    void clientChanged(ChatClient *client);

public slots:
    void addBookmark(const QUrl &url, const QString &name, const QUrl &oldUrl = QUrl());
    void removeBookmark(const QUrl &url);

private slots:
    void _q_bookmarksReceived();

private:
    enum Role {
        UrlRole = ChatModel::UserRole,
    };

    ChatClient *m_client;
};

#endif
