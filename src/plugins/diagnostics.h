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

#ifndef __WILINK_DIAGNOSTICS_H__
#define __WILINK_DIAGNOSTICS_H__

#include "QXmppClientExtension.h"

#include "chat_panel.h"
#include "diagnostics/iq.h"

class Chat;
class QTextBrowser;
class QTimer;

/** The DiagnosticsPanel represents panel for displaying diagnostics results.
 */
class DiagnosticsPanel : public ChatPanel
{
    Q_OBJECT

public:
    DiagnosticsPanel(Chat *chatWindow, QXmppClient *client);
    ~DiagnosticsPanel();

private slots:
    void refresh();
    void showResults(const DiagnosticsIq &iq);
    void slotShow();
    void timeout();

private:
    QAction *refreshAction;
    QTextBrowser *text;

    QXmppClient *m_client;
    bool m_displayed;
    QTimer *m_timer;
};

class DiagnosticsExtension : public QXmppClientExtension
{
    Q_OBJECT

public:
    DiagnosticsExtension(QXmppClient *client);

    void requestDiagnostics(const QString &jid);

    /// \cond
    QStringList discoveryFeatures() const;
    bool handleStanza(const QDomElement &element);
    /// \endcond

    static void lookup(const DiagnosticsIq &request, QObject *receiver, const char *member);

signals:
    void diagnosticsReceived(const DiagnosticsIq &diagnostics);

private slots:
    void diagnosticsServerFound(const QString &diagServer);
    void handleResults(const DiagnosticsIq &results);

private:
    QString m_diagnosticsServer;
};

class DiagnosticsAgent : public QObject
{
    Q_OBJECT

public:
    DiagnosticsAgent(QObject *parent = 0) : QObject(parent) {};

private slots:
    void handle(const DiagnosticsIq &request);
    void transfersFinished(const QList<Transfer> &transfers);

signals:
    void finished(const DiagnosticsIq &results);

private:
    DiagnosticsIq iq;
};

#endif
