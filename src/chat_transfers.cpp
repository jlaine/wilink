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

#define KIBIBYTE (1024)
#define MEBIBYTE (1024 * 1024)
#define GIGIBYTE (1024 * 1024 * 1024)

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

    QTableWidgetItem *sizeItem = new QTableWidgetItem(sizeToString(job->fileSize()));
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
    } else {
        jobs.at(jobRow)->abort();
    }
}

QSize ChatTransfers::sizeHint() const
{
    return QSize(400, 200);
}

QString ChatTransfers::sizeToString(qint64 size)
{
    if (size < KIBIBYTE)
        return QString::fromUtf8("%1 B").arg(size);
    else if (size < MEBIBYTE)
        return QString::fromUtf8("%1 KiB").arg(double(size) / double(KIBIBYTE), 0, 'f', 1);
    else if (size < GIGIBYTE)
        return QString::fromUtf8("%1 MiB").arg(double(size) / double(MEBIBYTE), 0, 'f', 1);
    else
        return QString::fromUtf8("%1 GiB").arg(double(size) / double(GIGIBYTE), 0, 'f', 1);
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
        removeButton->setIcon(QIcon(":/remove.png"));
    else
        removeButton->setIcon(QIcon(":/close.png"));
    removeButton->setEnabled(true);
}

