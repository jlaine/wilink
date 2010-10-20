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

#ifndef __WILINK_DIAGNOSTICS_H__
#define __WILINK_DIAGNOSTICS_H__

#include <QDialog>
#include <QHostInfo>
#include <QThread>
#include <QUrl>

#include "QXmppClientExtension.h"
#include "QXmppIq.h"

#include "chat_panel.h"
#include "diagnostics/networkinfo.h"
#include "diagnostics/wireless.h"

class QNetworkAccessManager;
class QPushButton;
class QProgressBar;
class QTextBrowser;

class NetworkThread;

class WirelessResult
{
public:
    QNetworkInterface interface;
    QList<WirelessNetwork> availableNetworks;
    WirelessNetwork currentNetwork;
    WirelessStandards supportedStandards;

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
};

class DiagnosticsIq : public QXmppIq
{
public:
    QList<WirelessResult> wirelessResults() const;
    void setWirelessResults(QList<WirelessResult> &wirelessResults);

    static bool isDiagnosticsIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QList<WirelessResult> m_wirelessResults;
};

class NetworkThread : public QThread
{
    Q_OBJECT

public:
    NetworkThread(QObject *parent) : QThread(parent) {};
    void run();

signals:
    void progress(int done, int total);
    void results(const DiagnosticsIq &results);

    void dnsResults(const QList<QHostInfo> &results);
    void pingResults(const QList<Ping> &results);
    void tracerouteResults(const QList<Ping> &results);
    void wirelessResult(const WirelessResult &result);
};

class Diagnostics : public ChatPanel
{
    Q_OBJECT

public:
    Diagnostics(QWidget *parent=0);
    void setUrl(const QUrl &url);

public slots:
    void slotShow();

protected slots:
    void addItem(const QString &title, const QString &value);
    void addSection(const QString &title);
    void refresh();
    void send();

    void showDns(const QList<QHostInfo> &results);
    void showPing(const QList<Ping> &results);
    void showProgress(int done, int total);
    void showTraceroute(const Traceroute &results);
    void showWireless(const WirelessResult &result);
    void networkFinished();

private:
    bool displayed;
    QProgressBar *progressBar;
    QPushButton *refreshButton;
    QTextBrowser *text;
    QPushButton *sendButton;
    QUrl diagnosticsUrl;
    QNetworkAccessManager *network;

    NetworkThread *networkThread;
};

class DiagnosticsExtension : public QXmppClientExtension
{
    Q_OBJECT

public:
    DiagnosticsExtension(QXmppClient *client);
    bool handleStanza(const QDomElement &stanza);

private slots:
    void handleResults(const DiagnosticsIq &results);
};

#endif
