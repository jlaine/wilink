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

#include "QXmppClientExtension.h"
#include "QXmppIq.h"

#include "chat_panel.h"
#include "diagnostics/interface.h"
#include "diagnostics/ping.h"
#include "diagnostics/wireless.h"

class QPushButton;
class QProgressBar;
class QTextBrowser;

class DiagnosticsThread;

class DiagnosticsIq : public QXmppIq
{
public:
    QList<Interface> interfaces() const;
    void setInterfaces(QList<Interface> &interfaces);

    QList<Ping> pings() const;
    void setPings(const QList<Ping> &pings);

    QList<Traceroute> traceroutes() const;
    void setTraceroutes(const QList<Traceroute> &traceroutes);

    static bool isDiagnosticsIq(const QDomElement &element);

protected:
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QList<Interface> m_interfaces;
    QList<Ping> m_pings;
    QList<Traceroute> m_traceroutes;
};

class Diagnostics : public ChatPanel
{
    Q_OBJECT

public:
    Diagnostics(QWidget *parent=0);

public slots:
    void slotShow();

protected slots:
    void addItem(const QString &title, const QString &value);
    void addSection(const QString &title);
    void refresh();

    void showDns(const QList<QHostInfo> &results);
    void showInterface(const Interface &result);
    void showPing(const QList<Ping> &results);
    void showProgress(int done, int total);
    void showTraceroute(const QList<Traceroute> &results);
    void networkFinished();

private:
    bool displayed;
    QProgressBar *progressBar;
    QPushButton *refreshButton;
    QTextBrowser *text;

    DiagnosticsThread *m_thread;
};

class DiagnosticsExtension : public QXmppClientExtension
{
    Q_OBJECT

public:
    DiagnosticsExtension(QXmppClient *client);
    ~DiagnosticsExtension();

    bool handleStanza(const QDomElement &stanza);
    void requestDiagnostics(const QString &jid);

signals:
    void diagnosticsReceived(const DiagnosticsIq &diagnostics);

private slots:
    void handleResults(const DiagnosticsIq &results);

private:
    DiagnosticsThread *m_thread;
};

class DiagnosticsThread : public QThread
{
    Q_OBJECT

public:
    DiagnosticsThread(QObject *parent) : QThread(parent) {};
    void run();
    void setId(const QString &id) { m_id = id; };
    void setTo(const QString &to) { m_to = to; };

signals:
    void progress(int done, int total);
    void results(const DiagnosticsIq &results);

    void dnsResults(const QList<QHostInfo> &results);
    void interfaceResult(const Interface &result);
    void pingResults(const QList<Ping> &results);
    void tracerouteResults(const QList<Traceroute> &results);

private:
    QString m_id;
    QString m_to;
};

#endif
