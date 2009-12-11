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

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

/** A TrayIcon is a system tray icon for interacting with a Panel.
 */
class Updates : public QObject
{
    Q_OBJECT

public:
    Updates(const QUrl &url, QObject *parent);

public slots:
    void check();

protected slots:
    void requestFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *network;
    QUrl statusUrl;
};

#endif
