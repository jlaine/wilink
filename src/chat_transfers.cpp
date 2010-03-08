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
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "qxmpp/QXmppUtils.h"

#include "chat_transfers.h"
#include "systeminfo.h"

#define KILOBYTE 1000
#define MEGABYTE 1000000
#define GIGABYTE 1000000000

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

ChatTransferPrompt::ChatTransferPrompt(QXmppTransferJob *job, const QString &contactName, QWidget *parent)
    : QMessageBox(parent), m_job(job)
{
    setIcon(QMessageBox::Question);
    setText(tr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?")
            .arg(contactName, job->fileName(), ChatTransfers::sizeToString(job->fileSize())));
    setWindowModality(Qt::NonModal);
    setWindowTitle(tr("File from %1").arg(contactName));

    setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    setDefaultButton(0);

    connect(this, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotButtonClicked(QAbstractButton*)));
}

void ChatTransferPrompt::slotButtonClicked(QAbstractButton *button)
{
    if (standardButton(button) == QMessageBox::Yes)
        emit fileAccepted(m_job);
    else
        emit fileDeclined(m_job);
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
    if (jobs.contains(job))
        return;

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

    connect(job, SIGNAL(finished()), this, SLOT(finished()));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(progress(qint64, qint64)));
    connect(job, SIGNAL(stateChanged(QXmppTransferJob::State)), this, SLOT(stateChanged(QXmppTransferJob::State)));

    show();
}

void ChatTransfers::cellDoubleClicked(int row, int column)
{
    if (row < 0 || row >= jobs.size())
        return;

    QXmppTransferJob *job = jobs.at(row);
    const QString localFilePath = job->data(LocalPathRole).toString();
    if (localFilePath.isEmpty())
        return;
    if (job->direction() == QXmppTransferJob::IncomingDirection &&
        (job->state() != QXmppTransferJob::FinishedState || job->error() != QXmppTransferJob::NoError))
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath));
}

void ChatTransfers::finished()
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    // if the job failed, reset the progress bar
    QProgressBar *progress = qobject_cast<QProgressBar*>(tableWidget->cellWidget(jobRow, ProgressColumn));
    if (progress && job->error() != QXmppTransferJob::NoError)
        progress->reset();
}

void ChatTransfers::fileAccepted(QXmppTransferJob *job)
{
    // determine file location
    QDir downloadsDir(SystemInfo::downloadsLocation());
    const QString filePath = QFileDialog::getSaveFileName(this, tr("Receive a file"),
        downloadsDir.absoluteFilePath(job->fileName()));
    if (filePath.isEmpty())
    {
        job->abort();
        return;
    }

    QFile *file = new QFile(filePath, job);
    if (file->open(QIODevice::WriteOnly))
    {
        job->setData(LocalPathRole, filePath);

        // show transfer window
        addJob(job);

        // start transfer
        job->accept(file);
    } else {
        job->abort();
    }
}

void ChatTransfers::fileDeclined(QXmppTransferJob *job)
{
    job->abort();
}

void ChatTransfers::fileReceived(QXmppTransferJob *job)
{
    const QString bareJid = jidToBareJid(job->jid());
//    const QString contactName = rosterModel->contactName(bareJid);

    // prompt user
    ChatTransferPrompt *dlg = new ChatTransferPrompt(job, bareJid, this);
    connect(dlg, SIGNAL(fileAccepted(QXmppTransferJob*)), this, SLOT(fileAccepted(QXmppTransferJob*)));
    connect(dlg, SIGNAL(fileDeclined(QXmppTransferJob*)), this, SLOT(fileDeclined(QXmppTransferJob*)));
    dlg->show();
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
    if (size < KILOBYTE)
        return QString::fromUtf8("%1 B").arg(size);
    else if (size < MEGABYTE)
        return QString::fromUtf8("%1 KB").arg(double(size) / double(KILOBYTE), 0, 'f', 1);
    else if (size < GIGABYTE)
        return QString::fromUtf8("%1 MB").arg(double(size) / double(MEGABYTE), 0, 'f', 1);
    else
        return QString::fromUtf8("%1 GB").arg(double(size) / double(GIGABYTE), 0, 'f', 1);
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

void ChatTransfers::toggle()
{
    if (isVisible())
        hide();
    else
        show();
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

