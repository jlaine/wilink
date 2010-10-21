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

#include "QXmppClientExtension.h"

#include "chat_panel.h"
#include "diagnostics/iq.h"

class QLineEdit;
class QPushButton;
class QProgressBar;
class QTextBrowser;

class DiagnosticsThread;

class Diagnostics : public ChatPanel
{
    Q_OBJECT

public:
    Diagnostics(QXmppClient *client, QWidget *parent=0);
    ~Diagnostics();

public slots:
    void slotShow();

private slots:
    void refresh();
    void showProgress(int done, int total);
    void showResults(const DiagnosticsIq &iq);

private:
    void addItem(const QString &title, const QString &value);
    void addSection(const QString &title);
    void showInterface(const Interface &result);
    void showLookup(const QList<QHostInfo> &results);

    QLineEdit *hostEdit;
    QProgressBar *progressBar;
    QPushButton *refreshButton;
    QTextBrowser *text;

    QXmppClient *m_client;
    bool m_displayed;
    DiagnosticsThread *m_agent;
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

signals:
    void diagnosticsReceived(const DiagnosticsIq &diagnostics);

private slots:
    void handleResults(const DiagnosticsIq &results);
};

class DiagnosticsThread : public QObject
{
    Q_OBJECT

public:
    static void lookup(const DiagnosticsIq &request, QObject *receiver, const char *member);

private:
    DiagnosticsThread(QObject *parent = 0) : QObject(parent) {};

private slots:
    void handle(const DiagnosticsIq &request);

signals:
    void progress(int done, int total);
    void results(const DiagnosticsIq &results);
};

#endif
