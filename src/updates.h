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

#ifndef __UPDATES_H__
#define __UPDATES_H__

#include <QFile>
#include <QMap>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class Release
{
public:
    QString changes;
    QString package;
    QUrl url;
    QString version;
    QMap<QString, QByteArray> hashes;
};

/** The Updates class handling checking for software updates
 *  and installing them.
 */
class Updates : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        BadHash,
        DownloadFailed,
        InsecureLocation,
        SaveFailed,
    } UpdatesError;

    Updates(QObject *parent);
    void download(const Release &release, const QString &dirPath);

    QUrl url() const;
    void setUrl(const QUrl &url);

    static int compareVersions(const QString &v1, const QString v2);

public slots:
    void check();

protected slots:
    void saveUpdate();
    void processStatus();

signals:
    void checkFailed(Updates::UpdatesError error, const QString &errorString);
    void updateAvailable(const Release &release);
    void updateDownloaded(const QUrl &url);
    void updateFailed(Updates::UpdatesError error, const QString &errorString);
    void updateProgress(qint64 done, qint64 total);

private:
    QNetworkAccessManager *network;
    QFile downloadFile;
    Release downloadRelease;
    QUrl updatesUrl;
};

#endif
