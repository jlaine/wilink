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

#ifndef QXMPPSHAREIQ_H
#define QXMPPSHAREIQ_H

#include <QDateTime>
#include <QHash>
#include <QVariant>

#include "QXmppIq.h"

extern const char* ns_shares;

class QXmppShareLocation
{
public:
    QXmppShareLocation(const QString &jid = QString(), const QString &node = QString());

    QString jid() const;
    void setJid(const QString &jid);

    QString node() const;
    void setNode(const QString &node);

    bool operator==(const QXmppShareLocation &other) const;
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
private:
    QString m_jid;
    QString m_node;
};

Q_DECLARE_METATYPE(QXmppShareLocation)

class QXmppShareLocationList : public QList<QXmppShareLocation>
{
public:
    QXmppShareLocationList();
    QXmppShareLocationList(const QXmppShareLocation &location);
    bool operator==(const QXmppShareLocationList &other) const;
};

Q_DECLARE_METATYPE(QXmppShareLocationList)

class QXmppShareItem
{
public:
    enum Type
    {
        FileItem,
        CollectionItem,
    };

    QXmppShareItem(Type type = QXmppShareItem::FileItem);
    QXmppShareItem(const QXmppShareItem &other);
    ~QXmppShareItem();

    QDateTime fileDate() const;
    void setFileDate(const QDateTime &date);

    QByteArray fileHash() const;
    void setFileHash(const QByteArray &hash);

    qint64 fileSize() const;
    void setFileSize(qint64 size);

    QXmppShareLocationList locations() const;
    void setLocations(const QXmppShareLocationList &locations);

    QString name() const;
    void setName(const QString &name);

    qint64 popularity() const;
    void setPopularity(qint64 popularity);

    void setType(Type type);
    Type type() const;

    QXmppShareItem *appendChild(const QXmppShareItem &child);
    void clearChildren();
    QXmppShareItem *child(int row);
    const QXmppShareItem *child(int row) const;
    QList<QXmppShareItem*> children();
    QXmppShareItem *insertChild(int row, const QXmppShareItem &child);
    void moveChild(int oldRow, int newRow);
    QXmppShareItem *parent();
    void removeChild(QXmppShareItem *item);
    int row() const;
    int size() const;

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

    bool operator==(const QXmppShareItem &other) const;

private:
    void operator=(const QXmppShareItem &other);
    QXmppShareItem *m_parent;
    QList<QXmppShareItem*> m_children;

    QString m_name;
    Type m_type;
    QXmppShareLocationList m_locations;
    qint64 m_popularity;

    // file-specific
    QDateTime m_date;
    QByteArray m_hash;
    qint64 m_size;
};

class QXmppShareSearchIq : public QXmppIq
{
public:
    QXmppShareSearchIq();

    QXmppShareItem &collection();
    const QXmppShareItem &collection() const;

    // maximum depth of the search
    int depth() const;
    void setDepth(int depth);

    // whether to return file hashes
    bool hash() const;
    void setHash(bool hash);

    // base node of the search
    QString node() const;
    void setNode(const QString &node);

    // query string of the search
    QString search() const;
    void setSearch(const QString &search);

    // tag for the search, must be included in response
    QString tag() const;
    void setTag(const QString &tag);

    static bool isShareSearchIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QXmppShareItem m_collection;
    int m_depth;
    bool m_hash;
    QString m_node;
    QString m_search;
    QString m_tag;
};

class QXmppShareGetIq : public QXmppIq
{
public:
    QString node() const;
    void setNode(const QString &node);

    QString sid() const;
    void setSid(const QString &sid);

    static bool isShareGetIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QString m_node;
    QString m_sid;
};

#endif
