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

#include <QWidget>

#include "qxmpp/QXmppTransferManager.h"

class QPushButton;
class QTableWidget;

const int LocalPathRole = Qt::UserRole;

class ChatTransfers : public QWidget
{
    Q_OBJECT

public:
    ChatTransfers(QWidget *parent = 0);
    void addJob(QXmppTransferJob *job);
    static QString sizeToString(qint64 size);

private slots:
    void cellDoubleClicked(int row, int column);
    void finished();
    void progress(qint64, qint64);
    void removeCurrentJob();
    void stateChanged(QXmppTransferJob::State state);
    void updateButtons();

protected:
    QSize sizeHint() const;

private:
    QPushButton *removeButton;
    QTableWidget *tableWidget;
    QList<QXmppTransferJob*> jobs;
};

#endif
