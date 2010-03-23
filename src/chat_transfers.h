/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_TRANSFERS_H__
#define __WDESKTOP_CHAT_TRANSFERS_H__

#include <QMessageBox>

#include "qxmpp/QXmppTransferManager.h"

#include "chat_panel.h"

class ChatSharesModel;
class ChatSharesView;
class QPushButton;
class QTableWidget;
class QXmppClient;
class QXmppShareItem;
class QXmppSiPubIq;

enum JobDataRoles
{
    LocalPathRole = Qt::UserRole,
    RemotePathRole,
};

class ChatTransferPrompt : public QMessageBox
{
    Q_OBJECT

public:
    ChatTransferPrompt(QXmppTransferJob *job, const QString &contactName, QWidget *parent = 0);

signals:
    void fileAccepted(QXmppTransferJob *job);
    void fileDeclined(QXmppTransferJob *job);

private slots:
    void slotButtonClicked(QAbstractButton *button);

private:
    QXmppTransferJob *m_job; 
};

class ChatTransfers : public ChatPanel
{
    Q_OBJECT

public:
    ChatTransfers(QXmppClient *xmppClient, QWidget *parent = 0);
    ~ChatTransfers();

    void addJob(QXmppTransferJob *job);
    void sendFile(const QString &fullJid);
    void setQueueModel(ChatSharesModel *model);

    static QString sizeToString(qint64 size);

protected:
    QSize sizeHint() const;

private slots:
    void fileAccepted(QXmppTransferJob *job, const QString &subdir = QString());
    void fileDeclined(QXmppTransferJob *job);
    void fileExpected(const QString &sid, const QString &path);
    void fileReceived(QXmppTransferJob *job);
    void cellDoubleClicked(int row, int column);
    void finished();
    void progress(qint64, qint64);
    void removeCurrentJob();
    void stateChanged(QXmppTransferJob::State state);
    void updateButtons();

private:
    ChatSharesView *queueView;
    QPushButton *removeButton;
    QTableWidget *tableWidget;
    QList<QXmppTransferJob*> jobs;
    QXmppClient *client;
    QHash<QString, QString> expectedStreams;
};

#endif
