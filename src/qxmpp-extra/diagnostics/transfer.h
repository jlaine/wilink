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

#ifndef __DIAGNOSTICS_TRANSFER_H__
#define __DIAGNOSTICS_TRANSFER_H__

#include <QNetworkReply>
#include <QTime>
#include <QUrl>
#include <QXmlStreamWriter>

class QDomElement;
class QNetworkAccessManager;

/** The Transfer class represents an HTTP transfer result.
 *
 *  All times are expressed in milliseconds.
 */
class Transfer
{
public:
    enum Direction
    {
        Upload = 0,
        Download = 1,
    };

    Transfer();

    Transfer::Direction direction() const;
    void setDirection(Transfer::Direction direction);

    QNetworkReply::NetworkError error() const;
    void setError(QNetworkReply::NetworkError error);

    qint64 size() const;
    void setSize(qint64 size);

    int time() const;
    void setTime(int downloadTime);

    QUrl url() const;
    void setUrl(const QUrl &url);

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

private:
    Transfer::Direction m_direction;
    QNetworkReply::NetworkError m_error;
    qint64 m_size;
    int m_time;
    QUrl m_url;
};

class TransferTester : public QObject
{
    Q_OBJECT

public:
    TransferTester(QObject *parent = 0);
    void start(const QUrl &url);

signals:
    void finished(const QList<Transfer> &transfer);

private slots:
    void connectFinished();
    void downloadFinished();
    void uploadFinished();

private:
    QTime m_time;
    Transfer m_upload;
    Transfer m_download;
    QNetworkAccessManager *m_network;
};

#endif
