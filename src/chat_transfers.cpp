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

#include <QDebug>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "chat_transfers.h"

enum TransfersColumns {
    ProgressColumn = 0,
    NameColumn,
    SizeColumn,
    MaxColumn,
};

ChatTransfers::ChatTransfers(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    tableWidget = new QTableWidget;
    tableWidget->setColumnCount(MaxColumn);
    tableWidget->setHorizontalHeaderItem(NameColumn, new QTableWidgetItem(tr("File name")));
    tableWidget->setHorizontalHeaderItem(SizeColumn, new QTableWidgetItem(tr("Size")));
    tableWidget->setHorizontalHeaderItem(ProgressColumn, new QTableWidgetItem(tr("Progress")));
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setResizeMode(NameColumn, QHeaderView::Stretch);
    layout->addWidget(tableWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    QPushButton *removeButton = new QPushButton;
    removeButton->setIcon(QIcon(":/remove.png"));
    //connect(removeButton, SIGNAL(clicked()), this, SLOT(removeJob()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);
}

void ChatTransfers::addJob(QXmppTransferJob *job)
{
    jobs.insert(0, job);
    tableWidget->insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(job->fileName());
    tableWidget->setItem(0, NameColumn, nameItem);

    QString fileSize;
    if (job->fileSize() < 1024)
        fileSize = QString("%1 B").arg(job->fileSize());
    else if (job->fileSize() < 1024 * 1024)
        fileSize = QString("%1 KiB").arg(job->fileSize() / 1024);
    else
        fileSize = QString("%1 MiB").arg(job->fileSize() / (1024*1024));
    tableWidget->setItem(0, SizeColumn, new QTableWidgetItem(fileSize));

    QProgressBar *progress = new QProgressBar;
    progress->setMaximum(job->fileSize());
    tableWidget->setCellWidget(0, ProgressColumn, progress);

    connect(job, SIGNAL(error(QXmppTransferJob::Error)), this, SLOT(error(QXmppTransferJob::Error)));
    connect(job, SIGNAL(finished()), this, SLOT(finished()));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(progress(qint64, qint64)));
}

void ChatTransfers::removeJob(QXmppTransferJob *job)
{
    int jobRow = jobs.indexOf(job);
    if (jobRow < 0)
        return;

    jobs.removeAt(jobRow);
    tableWidget->removeRow(jobRow);
}

void ChatTransfers::error(QXmppTransferJob::Error error)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    qDebug() << "Job" << job->fileName() << "failed";
}

void ChatTransfers::finished()
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    qDebug() << "Job" << job->fileName() << "finished";
}

void ChatTransfers::progress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    QProgressBar *progress = qobject_cast<QProgressBar*>(tableWidget->cellWidget(jobRow, ProgressColumn));
    if (progress)
        progress->setValue(done);
}

QSize ChatTransfers::sizeHint() const
{
    return QSize(400, 200);
}
