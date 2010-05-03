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

#ifndef __WILINK_TRANSFERS_H__
#define __WILINK_TRANSFERS_H__

#include <QMessageBox>
#include <QTableWidget>

#include "qxmpp/QXmppTransferManager.h"

#include "chat_panel.h"

class QPushButton;
class QXmppClient;

enum JobDataRoles
{
    LocalPathRole = Qt::UserRole,
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

class ChatTransfersView : public QTableWidget
{
    Q_OBJECT

public:
    ChatTransfersView(QWidget *parent = 0);
    int activeJobs(QXmppTransferJob::Direction direction) const;
    void addJob(QXmppTransferJob *job);
    QXmppTransferJob *currentJob();

public slots:
    void removeCurrentJob();

signals:
    void updateButtons();

private slots:
    void slotDestroyed(QObject *object);
    void slotDoubleClicked(int row, int column);
    void slotFinished();
    void slotProgress(qint64, qint64);
    void slotStateChanged(QXmppTransferJob::State state);

private:
    QList<QXmppTransferJob*> jobs;
};

class ChatTransfers : public ChatPanel
{
    Q_OBJECT

public:
    ChatTransfers(QXmppClient *xmppClient, QWidget *parent = 0);
    ~ChatTransfers();

    int activeJobs(QXmppTransferJob::Direction direction) const;
    void addJob(QXmppTransferJob *job);

    static QString availableFilePath(const QString &dirPath, const QString &fileName);

public slots:
    void sendFile(const QString &fullJid);

protected:
    QSize sizeHint() const;

private slots:
    void fileAccepted(QXmppTransferJob *job);
    void fileDeclined(QXmppTransferJob *job);
    void fileReceived(QXmppTransferJob *job);
    void updateButtons();

private:
    QPushButton *removeButton;
    ChatTransfersView *tableWidget;
    QXmppClient *client;
};

#endif
