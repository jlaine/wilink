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

#ifndef __WILINK_TRANSFERS_H__
#define __WILINK_TRANSFERS_H__

#include <QMessageBox>
#include <QTableWidget>

#include "QXmppTransferManager.h"

#include "chat_panel.h"

class QModelIndex;
class Chat;

/** The ChatTransferPrompt class represents a message box to
 *  ask the user whether to accept an incoming transfer.
 */
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

class ChatTransfersWatcher : public QObject
{
    Q_OBJECT

public:
    ChatTransfersWatcher(Chat *chatWindow);
    bool eventFilter(QObject *obj, QEvent *e);

public slots:
    void addJob(QXmppTransferJob *job);
    void sendFile(const QString &fullJid, const QString &filePath);

private slots:
    void fileReceived(QXmppTransferJob *job);
    void sendFileAccepted(const QString &filePath);
    void sendFilePrompt();

private:
    Chat *chatWindow;
    QXmppTransferManager *transferManager;
};

#endif
