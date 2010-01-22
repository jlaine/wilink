/*
 * Copyright (C) 2010 Bolloré telecom
 *
 * Author:
 *	Jeremy Lainé
 *
 * Source:
 *	http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "QXmppArchiveIq.h"
#include "QXmppConstants.h"
#include "QXmppUtils.h"

#include <QDomElement>

// FIXME : RFC says "urn:xmpp:archive"
static const char *ns_archive = "http://www.xmpp.org/extensions/xep-0136.html#ns";

QXmppArchiveListIq::QXmppArchiveListIq() : QXmppIq(QXmppIq::Get)
{
}

QString QXmppArchiveListIq::getWith() const
{
    return m_with;
}

void QXmppArchiveListIq::setWith( const QString &with )
{
    m_with = with;
}

bool QXmppArchiveListIq::isArchiveListIq( QDomElement &element )
{
    QString type = element.attribute("type");
    QDomElement errorElement = element.firstChildElement("error");
    QDomElement listElement = element.firstChildElement("list");
    return (type == "error") &&
            !errorElement.isNull() &&
            listElement.namespaceURI() == ns_archive;
}

void QXmppArchiveListIq::parse( QDomElement &element )
{
    QDomElement listElement = element.firstChildElement("list");
    m_with = element.attribute("with");
}

void QXmppArchiveListIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("list");
    helperToXmlAddAttribute(writer, "xmlns", ns_archive);
    if (!m_with.isEmpty())
        helperToXmlAddAttribute(writer, "with", m_with);
    writer->writeStartElement("set");
    helperToXmlAddAttribute(writer, "xmlns", "http://jabber.org/protocol/rsm");
    helperToXmlAddTextElement(writer, "max", "30");
    writer->writeEndElement();
    writer->writeEndElement();
}

bool QXmppArchivePrefIq::isArchivePrefIq( QDomElement &element )
{
    QString type = element.attribute("type");
    QDomElement errorElement = element.firstChildElement("error");
    QDomElement prefElement = element.firstChildElement("pref");
    return (type == "error") &&
            !errorElement.isNull() &&
            prefElement.namespaceURI() == ns_archive;
}

void QXmppArchivePrefIq::parse( QDomElement &element )
{
    QDomElement queryElement = element.firstChildElement("pref");
    //setId( element.attribute("id"));
}

void QXmppArchivePrefIq::toXmlElementFromChild( QXmlStreamWriter *writer ) const
{
    writer->writeStartElement("pref");
    helperToXmlAddAttribute(writer, "xmlns", ns_archive);
    writer->writeEndElement();
}

