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

#include <QDomElement>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include "transfer.h"

Transfer::Transfer()
    : m_direction(Transfer::Download),
    m_error(QNetworkReply::NoError),
    m_size(0),
    m_time(0)
{
}

Transfer::Direction Transfer::direction() const
{
    return m_direction;
}

void Transfer::setDirection(Transfer::Direction direction)
{
    m_direction = direction;
}

QNetworkReply::NetworkError Transfer::error() const
{
    return m_error;
}

void Transfer::setError(QNetworkReply::NetworkError error)
{
    m_error = error;
}

qint64 Transfer::size() const
{
    return m_size;
}

void Transfer::setSize(qint64 size)
{
    m_size = size;
}

int Transfer::time() const
{
    return m_time;
}

void Transfer::setTime(int time)
{
    m_time = time;
}

QUrl Transfer::url() const
{
    return m_url;
}

void Transfer::setUrl(const QUrl &url)
{
    m_url = url;
}

void Transfer::parse(const QDomElement &element)
{
    m_direction = (element.attribute("direction") == "upload") ? Transfer::Upload : Transfer::Download;
    m_error = static_cast<QNetworkReply::NetworkError>(element.attribute("error").toInt());
    m_size = element.attribute("size").toLongLong();
    m_time = element.attribute("time").toInt();
    m_url = QUrl(element.attribute("url"));
}

void Transfer::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("transfer");
    writer->writeAttribute("direction", (m_direction == Transfer::Upload) ? "upload" : "download");
    writer->writeAttribute("error", QString::number(m_error));
    writer->writeAttribute("size", QString::number(m_size));
    writer->writeAttribute("time", QString::number(m_time));
    writer->writeAttribute("url", m_url.toString());
    writer->writeEndElement();
}

TransferTester::TransferTester(QObject *parent)
    : QObject(parent)
{
    m_network = new QNetworkAccessManager(this);
    m_download.setDirection(Transfer::Download);
    m_upload.setDirection(Transfer::Upload);
}

void TransferTester::start(const QUrl &url)
{
    // test connect
    QNetworkReply *connectReply = m_network->head(QNetworkRequest(url));
    connect(connectReply, SIGNAL(finished()), this, SLOT(connectFinished()));
    m_time.start();
}

void TransferTester::connectFinished()
{
    // process connect
    QNetworkReply *connectReply = qobject_cast<QNetworkReply*>(sender());
    if (!connectReply)
        return;
    if (connectReply->error() != QNetworkReply::NoError)
    {
        emit finished(QList<Transfer>());
        return;
    }

    // test download
    m_download.setUrl(connectReply->url());
    m_time.start();
    QNetworkReply *downloadReply = m_network->get(QNetworkRequest(m_download.url()));
    connect(downloadReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void TransferTester::downloadFinished()
{
    // process download
    QNetworkReply *downloadReply = qobject_cast<QNetworkReply*>(sender());
    if (!downloadReply)
        return;
    m_download.setTime(m_time.elapsed());
    m_download.setError(downloadReply->error());
    const QByteArray data = downloadReply->readAll();
    m_download.setSize(data.size());
    if (m_download.error() != QNetworkReply::NoError)
    {
        emit finished(QList<Transfer>() << m_download);
        return;
    }

    // test upload
    m_upload.setUrl(downloadReply->url());
    m_upload.setSize(data.size());
    m_time.start();
    QNetworkReply *uploadReply = m_network->post(QNetworkRequest(m_upload.url()), data);
    connect(uploadReply, SIGNAL(finished()), this, SLOT(uploadFinished()));
}

void TransferTester::uploadFinished()
{
    // process upload
    QNetworkReply *uploadReply = qobject_cast<QNetworkReply*>(sender());
    if (!uploadReply)
        return;
    m_upload.setTime(m_time.elapsed());
    m_upload.setError(uploadReply->error());
    emit finished(QList<Transfer>() << m_download << m_upload);
}
