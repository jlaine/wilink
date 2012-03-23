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

#include <QBuffer>
#include <QDomElement>

#include "QXmppShareIq.h"
#include "QXmppUtils.h"

const char* ns_shares = "http://wifirst.net/protocol/shares";
const char* ns_shares_get = "http://wifirst.net/protocol/shares#get";
const char* ns_shares_search = "http://wifirst.net/protocol/shares#search";

QXmppShareLocation::QXmppShareLocation(const QString &jid, const QString &node)
    : m_jid(jid), m_node(node)
{
}

QString QXmppShareLocation::jid() const
{
    return m_jid;
}

void QXmppShareLocation::setJid(const QString &jid)
{
    m_jid = jid;
}

QString QXmppShareLocation::node() const
{
    return m_node;
}

void QXmppShareLocation::setNode(const QString &node)
{
    m_node = node;
}

bool QXmppShareLocation::operator==(const QXmppShareLocation &other) const
{
    return other.m_jid == m_jid && other.m_node == m_node;
}

void QXmppShareLocation::parse(const QDomElement &element)
{
    m_jid = element.attribute("jid");
    m_node = element.attribute("node");
}

void QXmppShareLocation::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("location");
    helperToXmlAddAttribute(writer, "jid", m_jid);
    helperToXmlAddAttribute(writer, "node", m_node);
    writer->writeEndElement();
}

QXmppShareLocationList::QXmppShareLocationList()
{
}

QXmppShareLocationList::QXmppShareLocationList(const QXmppShareLocation &location)
{
    append(location);
}

/// Two location lists are considered equal if they have at least
/// one location in common.

bool QXmppShareLocationList::operator==(const QXmppShareLocationList &other) const
{
    foreach (const QXmppShareLocation &location, *this)
        if (other.contains(location))
            return true;
    return false;
}

QXmppShareItem::QXmppShareItem(Type type)
    : m_parent(0), m_type(type), m_popularity(0), m_size(0)
{
}

QXmppShareItem::QXmppShareItem(const QXmppShareItem &other)
    : m_parent(0),
    m_name(other.m_name),
    m_type(other.m_type),
    m_locations(other.m_locations),
    m_popularity(other.m_popularity),
    m_date(other.m_date),
    m_hash(other.m_hash),
    m_size(other.m_size)
{
    foreach (const QXmppShareItem *child, other.m_children)
        appendChild(*child);
}

QXmppShareItem::~QXmppShareItem()
{
    foreach (QXmppShareItem *child, m_children)
        delete child;
}

QXmppShareItem *QXmppShareItem::appendChild(const QXmppShareItem &child)
{
    QXmppShareItem *copy = new QXmppShareItem(child);
    copy->m_parent = this;
    m_children.append(copy);
    return copy;
}

QXmppShareItem *QXmppShareItem::child(int row)
{
    if (row >= 0 && row < m_children.size())
        return m_children[row];
    else
        return 0;
}

const QXmppShareItem *QXmppShareItem::child(int row) const
{
    if (row >= 0 && row < m_children.size())
        return m_children[row];
    else
        return 0;
}

QList<QXmppShareItem*> QXmppShareItem::children()
{
    return m_children;
}

void QXmppShareItem::clearChildren()
{
    m_children.clear();
}

QDateTime QXmppShareItem::fileDate() const
{
    return m_date;
}

void QXmppShareItem::setFileDate(const QDateTime &date)
{
    m_date = date;
}

QByteArray QXmppShareItem::fileHash() const
{
    return m_hash;
}

void QXmppShareItem::setFileHash(const QByteArray &hash)
{
    m_hash = hash;
}

qint64 QXmppShareItem::fileSize() const
{
    return m_size;
}

void QXmppShareItem::setFileSize(qint64 size)
{
    m_size = size;
}

QXmppShareItem *QXmppShareItem::insertChild(int i, const QXmppShareItem &child)
{
    QXmppShareItem *copy = new QXmppShareItem(child);
    copy->m_parent = this;
    m_children.insert(i, copy);
    return copy;
}

QXmppShareLocationList QXmppShareItem::locations() const
{
    return m_locations;
}

void QXmppShareItem::setLocations(const QXmppShareLocationList &locations)
{
    m_locations = locations;
}

void QXmppShareItem::moveChild(int oldRow, int newRow)
{
    m_children.move(oldRow, newRow);
}

QString QXmppShareItem::name() const
{
    return m_name;
}

void QXmppShareItem::setName(const QString &name)
{
    m_name = name;
}

QXmppShareItem *QXmppShareItem::parent()
{
    return m_parent;
}

qint64 QXmppShareItem::popularity() const
{
    return m_popularity;
}

void QXmppShareItem::setPopularity(qint64 popularity)
{
    m_popularity = popularity;
}

void QXmppShareItem::removeChild(QXmppShareItem *child)
{
    const int index = m_children.indexOf(child);
    if (index >= 0)
    {
        m_children.removeAt(index);
        delete child;
    }
}

int QXmppShareItem::row() const
{
    if (!m_parent)
        return 0;
    return m_parent->m_children.indexOf((QXmppShareItem*)this);
}

int QXmppShareItem::size() const
{
    return m_children.size();
}

void QXmppShareItem::setType(QXmppShareItem::Type type)
{
    m_type = type;
}

QXmppShareItem::Type QXmppShareItem::type() const
{
    return m_type;
}

bool QXmppShareItem::operator==(const QXmppShareItem &other) const
{
    if (other.fileSize() != fileSize() || other.fileHash() != fileHash())
        return false;
    if (other.m_children.size() != m_children.size())
        return false;
    for (int i = 0; i < m_children.size(); i++)
        if (!(other.m_children[i] == m_children[i]))
            return false;
    return true;
}

void QXmppShareItem::parse(const QDomElement &element)
{
    if (element.tagName() == "collection")
        m_type = CollectionItem;
    else
        m_type = FileItem;

    m_date = datetimeFromString(element.attribute("date"));
    m_hash = QByteArray::fromHex(element.attribute("hash").toAscii());
    m_name = element.attribute("name");
    m_popularity = element.attribute("popularity").toLongLong();
    m_size = element.attribute("size").toLongLong();
    QDomElement childElement = element.firstChildElement();
    while (!childElement.isNull())
    {
        if (childElement.tagName() == "collection")
        {
            QXmppShareItem item(CollectionItem);
            item.parse(childElement);
            appendChild(item);
        }
        else if (childElement.tagName() == "file")
        {
            QXmppShareItem item(FileItem);
            item.parse(childElement);
            appendChild(item);
        } else if (childElement.tagName() == "location") {
            QXmppShareLocation location;
            location.parse(childElement);
            m_locations.append(location);
        }
        childElement = childElement.nextSiblingElement();
    }
}

void QXmppShareItem::toXml(QXmlStreamWriter *writer) const
{
    if (m_type == CollectionItem)
        writer->writeStartElement("collection");
    else
        writer->writeStartElement("file");
    helperToXmlAddAttribute(writer, "date", datetimeToString(m_date));
    helperToXmlAddAttribute(writer, "name", m_name);
    helperToXmlAddAttribute(writer, "hash", m_hash.toHex());
    if (m_popularity)
        helperToXmlAddAttribute(writer, "popularity", QString::number(m_popularity));
    if (m_size)
        helperToXmlAddAttribute(writer, "size", QString::number(m_size));
    foreach (const QXmppShareItem *item, m_children)
        item->toXml(writer);
    foreach (const QXmppShareLocation &location, locations())
        location.toXml(writer);
    writer->writeEndElement();
}

QXmppShareSearchIq::QXmppShareSearchIq()
    : m_collection(QXmppShareItem::CollectionItem), m_depth(0), m_hash(0)
{
    m_tag = generateStanzaHash();
}

QXmppShareItem &QXmppShareSearchIq::collection()
{
    return m_collection;
}

const QXmppShareItem &QXmppShareSearchIq::collection() const
{
    return m_collection;
}

bool QXmppShareSearchIq::hash() const
{
    return m_hash;
}

void QXmppShareSearchIq::setHash(bool hash)
{
    m_hash = hash;
}

int QXmppShareSearchIq::depth() const
{
    return m_depth;
}

void QXmppShareSearchIq::setDepth(int depth)
{
    m_depth = depth;
}

QString QXmppShareSearchIq::node() const
{
    return m_node;
}

void QXmppShareSearchIq::setNode(const QString &node)
{
    m_node = node;
}

QString QXmppShareSearchIq::search() const
{
    return m_search;
}

void QXmppShareSearchIq::setSearch(const QString &search)
{
    m_search = search;
}

QString QXmppShareSearchIq::tag() const
{
    return m_tag;
}

void QXmppShareSearchIq::setTag(const QString &tag)
{
    m_tag = tag;
}

bool QXmppShareSearchIq::isShareSearchIq(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    return (queryElement.namespaceURI() == ns_shares_search);
}

void QXmppShareSearchIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    m_depth = queryElement.attribute("depth").toInt();
    m_hash = queryElement.attribute("hash") == "1";
    m_node = queryElement.attribute("node");
    m_search = queryElement.attribute("search");
    m_tag = queryElement.attribute("tag");

    // decompress query content
    if (!queryElement.text().isEmpty())
    {
        const QByteArray data = qUncompress(QByteArray::fromBase64(queryElement.text().toAscii()));
        QDomDocument doc;
        doc.setContent(data);
        m_collection.parse(doc.documentElement());
    }
}

void QXmppShareSearchIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("query");
    writer->writeAttribute("xmlns", ns_shares_search);
    helperToXmlAddAttribute(writer, "node", m_node);
    if (m_depth > 0)
        helperToXmlAddAttribute(writer, "depth", QString::number(m_depth));
    if (m_hash)
        helperToXmlAddAttribute(writer, "hash", "1");
    helperToXmlAddAttribute(writer, "search", m_search);
    helperToXmlAddAttribute(writer, "tag", m_tag);

    // compress query content
    if (m_collection.size() || !m_collection.locations().isEmpty())
    {
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        QXmlStreamWriter queryWriter(&buffer);
        m_collection.toXml(&queryWriter);
        writer->writeCharacters(qCompress(buffer.data()).toBase64());
    }
    writer->writeEndElement();
}

QString QXmppShareGetIq::node() const
{
    return m_node;
}

void QXmppShareGetIq::setNode(const QString &node)
{
    m_node = node;
}

QString QXmppShareGetIq::sid() const
{
    return m_sid;
}

void QXmppShareGetIq::setSid(const QString &sid)
{
    m_sid = sid;
}

bool QXmppShareGetIq::isShareGetIq(const QDomElement &element)
{
    return (element.firstChildElement("query").namespaceURI() == ns_shares_get);
}

void QXmppShareGetIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    m_node = queryElement.attribute("node");
    m_sid = queryElement.attribute("sid");
}

void QXmppShareGetIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("query");
    writer->writeAttribute("xmlns", ns_shares_get);
    helperToXmlAddAttribute(writer, "node", m_node);
    helperToXmlAddAttribute(writer, "sid", m_sid);
    writer->writeEndElement();
}

