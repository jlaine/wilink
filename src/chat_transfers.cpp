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
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "chat_transfers.h"

enum TransfersColumns {
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

static QIcon jobIcon(QXmppTransferJob *job)
{
    if (job->state() == QXmppTransferJob::FinishedState)
    {
        if (job->error() == QXmppTransferJob::NoError)
            return QIcon(":/contact-available.png");
        else
            return QIcon(":/contact-busy.png");
    }
    return QIcon(":/contact-offline.png");
}

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
    tableWidget->setShowGrid(false);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setResizeMode(NameColumn, QHeaderView::Stretch);
    connect(tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)));
    connect(tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(updateButtons()));
    layout->addWidget(tableWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    removeButton = new QPushButton;
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeCurrentJob()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);
    updateButtons();
}

void ChatTransfers::addJob(QXmppTransferJob *job)
{
    jobs.insert(0, job);
    tableWidget->insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(job->fileName());
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    nameItem->setIcon(jobIcon(job));
    tableWidget->setItem(0, NameColumn, nameItem);

    QString fileSize;
    if (job->fileSize() < 1024)
        fileSize = QString("%1 B").arg(job->fileSize());
    else if (job->fileSize() < 1024 * 1024)
        fileSize = QString("%1 KiB").arg(job->fileSize() / 1024);
    else
        fileSize = QString("%1 MiB").arg(job->fileSize() / (1024*1024));
    QTableWidgetItem *sizeItem = new QTableWidgetItem(fileSize);
    sizeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    tableWidget->setItem(0, SizeColumn, sizeItem);

    QProgressBar *progress = new QProgressBar;
    progress->setMaximum(job->fileSize());
    tableWidget->setCellWidget(0, ProgressColumn, progress);

    connect(job, SIGNAL(error(QXmppTransferJob::Error)), this, SLOT(error(QXmppTransferJob::Error)));
    connect(job, SIGNAL(finished()), this, SLOT(finished()));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(progress(qint64, qint64)));
    connect(job, SIGNAL(stateChanged(QXmppTransferJob::State)), this, SLOT(stateChanged(QXmppTransferJob::State)));
}

void ChatTransfers::cellDoubleClicked(int row, int column)
{
    if (row < 0 || row >= jobs.size())
        return;

    QXmppTransferJob *job = jobs.at(row);
    if (job->localFilePath().isEmpty())
        return;
    if (job->direction() == QXmppTransferJob::IncomingDirection &&
        (job->state() != QXmppTransferJob::FinishedState || job->error() != QXmppTransferJob::NoError))
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(job->localFilePath()));
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

void ChatTransfers::removeCurrentJob()
{
    int jobRow = tableWidget->currentRow();
    if (jobRow < 0 || jobRow >= jobs.size())
        return;

    QXmppTransferJob *job = jobs.at(jobRow);
    if (job->state() == QXmppTransferJob::FinishedState)
    {
        jobs.removeAt(jobRow);
        tableWidget->removeRow(jobRow);
        updateButtons();
    }
}

QSize ChatTransfers::sizeHint() const
{
    return QSize(400, 200);
}

void ChatTransfers::stateChanged(QXmppTransferJob::State state)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    tableWidget->item(jobRow, NameColumn)->setIcon(jobIcon(job));
    updateButtons();
}

void ChatTransfers::updateButtons()
{
    int jobRow = tableWidget->currentRow();
    if (jobRow < 0 || jobRow >= jobs.size())
    {
        removeButton->setEnabled(false);
        removeButton->setIcon(QIcon(":/remove.png"));
        return;
    }

    QXmppTransferJob *job = jobs.at(jobRow);
    if (job->state() == QXmppTransferJob::FinishedState)
    {
        removeButton->setEnabled(true);
        removeButton->setIcon(QIcon(":/remove.png"));
    } else {
        removeButton->setEnabled(false);
        removeButton->setIcon(QIcon(":/close.png"));
    }
}

