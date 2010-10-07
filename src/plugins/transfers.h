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

#include "QXmppTransferManager.h"

#include "chat_panel.h"

class QModelIndex;
class QProgressBar;
class QPushButton;
class QVBoxLayout;
class ChatRosterModel;

class ChatTransferPrompt : public QMessageBox
{
    Q_OBJECT

public:
    ChatTransferPrompt(QXmppTransferJob *job, const QString &contactName, QWidget *parent = 0);

signals:
    void fileAccepted(QXmppTransferJob *job);

private slots:
    void slotButtonClicked(QAbstractButton *button);

private:
    QXmppTransferJob *m_job;
};

class ChatTransferWidget : public QWidget
{
    Q_OBJECT

public:
    ChatTransferWidget(QXmppTransferJob *job, QWidget *parent = 0);

protected:
    void mouseDoubleClickEvent(QMouseEvent * event);

private slots:
    void slotCancel();
    void slotDestroyed(QObject *object);
    void slotProgress(qint64, qint64);
    void slotStateChanged(QXmppTransferJob::State state);

private:
    QLabel *m_icon;
    QProgressBar *m_progress;
    QPushButton *m_cancelButton;
    QXmppTransferJob *m_job;
};

class ChatTransfersView : public QTableWidget
{
    Q_OBJECT

public:
    ChatTransfersView(QWidget *parent = 0);
    QXmppTransferJob *currentJob();

public slots:
    void addJob(QXmppTransferJob *job);
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
    ChatTransfers(QXmppClient *xmppClient, ChatRosterModel *rosterModel, QWidget *parent = 0);
    ~ChatTransfers();

public slots:
    void sendFile(const QString &fullJid, const QString &filePath);
    void rosterDrop(QDropEvent *event, const QModelIndex &index);
    void rosterMenu(QMenu *menu, const QModelIndex &index);

protected:
    QSize sizeHint() const;

private slots:
    void fileReceived(QXmppTransferJob *job);
    void sendFileAccepted(const QString &filePath);
    void sendFilePrompt();
    void updateButtons();

private:
    QPushButton *removeButton;
    ChatTransfersView *tableWidget;
    QVBoxLayout *tableLayout;
    QXmppClient *client;
    ChatRosterModel *rosterModel;
};

#endif
