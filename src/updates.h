/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class Release
{
public:
    QString description;
    QString package;
    QUrl url;
    QString version;
};

/** The Updates class handling checking for software updates
 *  and installing them.
 */
class Updates : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Idle,
        Download,
        Install,
    };

    Updates(QObject *parent);
    void check(const QUrl &url, const QString &version);
    void install(const Release &release);

    static int compareVersions(const QString &v1, const QString v2);
    static QString platform();

protected slots:
    void installUpdate();
    void saveUpdate();
    void processStatus();

signals:
    void updateAvailable(const Release &release);
    void updateComplete();
    void updateFailed();
    void updateProgress(qint64 done, qint64 total);
    void updateStatus(int status);

private:
    QNetworkAccessManager *network;
    QString currentVersion;
    QFile downloadFile;
};

#endif
