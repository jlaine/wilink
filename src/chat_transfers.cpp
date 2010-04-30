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

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTime>
#include <QUrl>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppUtils.h"

#include "chat_transfers.h"
#include "systeminfo.h"

#define KILOBYTE 1000
#define MEGABYTE 1000000
#define GIGABYTE 1000000000

static qint64 fileSizeLimit = 10000000; // 10 MB

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
    setDefaultButton(QMessageBox::NoButton);

    /* connect signals */
    connect(this, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(slotButtonClicked(QAbstractButton*)));
}

void ChatTransferPrompt::slotButtonClicked(QAbstractButton *button)
{
    if (standardButton(button) == QMessageBox::Yes)
        emit fileAccepted(m_job);
    else
        emit fileDeclined(m_job);
}

ChatTransfersView::ChatTransfersView(QWidget *parent)
    : QTableWidget(parent)
{
    setColumnCount(MaxColumn);
    setHorizontalHeaderItem(NameColumn, new QTableWidgetItem(tr("File name")));
    setHorizontalHeaderItem(SizeColumn, new QTableWidgetItem(tr("Size")));
    setHorizontalHeaderItem(ProgressColumn, new QTableWidgetItem(tr("Progress")));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setShowGrid(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setResizeMode(NameColumn, QHeaderView::Stretch);
    
    connect(this, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(slotDoubleClicked(int,int)));
}

int ChatTransfersView::activeJobs(QXmppTransferJob::Direction direction) const
{
    int active = 0;
    foreach (QXmppTransferJob *job, jobs)
        if (job->direction() == direction &&
            job->state() != QXmppTransferJob::FinishedState)
            active++;
    return active;
}

void ChatTransfersView::addJob(QXmppTransferJob *job)
{
    if (jobs.contains(job))
        return;

    const QString fileName = QFileInfo(job->data(LocalPathRole).toString()).fileName();

    jobs.insert(0, job);
    insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileName);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    nameItem->setIcon(jobIcon(job));
    setItem(0, NameColumn, nameItem);

    QTableWidgetItem *sizeItem = new QTableWidgetItem(ChatTransfers::sizeToString(job->fileSize()));
    sizeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setItem(0, SizeColumn, sizeItem);

    QProgressBar *progress = new QProgressBar;
    progress->setMaximum(job->fileSize());
    setCellWidget(0, ProgressColumn, progress);

    connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyed(QObject*)));
    connect(job, SIGNAL(finished()), this, SLOT(slotFinished()));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(slotProgress(qint64, qint64)));
    connect(job, SIGNAL(stateChanged(QXmppTransferJob::State)), this, SLOT(slotStateChanged(QXmppTransferJob::State)));
}

QXmppTransferJob *ChatTransfersView::currentJob()
{
    int jobRow = currentRow();
    if (jobRow < 0 || jobRow >= jobs.size())
        return 0;
    return jobs.at(jobRow);
}

void ChatTransfersView::removeCurrentJob()
{
    QXmppTransferJob *job = currentJob();
    if (!job)
        return;
    if (job->state() == QXmppTransferJob::FinishedState)
        job->deleteLater();
    else
        job->abort();
}

void ChatTransfersView::slotDestroyed(QObject *obj)
{
    int jobRow = jobs.indexOf(static_cast<QXmppTransferJob*>(obj));
    if (jobRow < 0)
        return;

    jobs.removeAt(jobRow);
    removeRow(jobRow);
    emit updateButtons();
}

void ChatTransfersView::slotDoubleClicked(int row, int column)
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

void ChatTransfersView::slotFinished()
{
    // find the job that completed
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    // if the job failed, reset the progress bar
    const QString localFilePath = job->data(LocalPathRole).toString();
    QProgressBar *progress = qobject_cast<QProgressBar*>(cellWidget(jobRow, ProgressColumn));
    if (progress && job->error() != QXmppTransferJob::NoError)
    {
        progress->reset();

        // delete failed downloads
        if (job->direction() == QXmppTransferJob::IncomingDirection &&
            !localFilePath.isEmpty())
            QFile(localFilePath).remove();
    }
}

void ChatTransfersView::slotProgress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    QProgressBar *progress = qobject_cast<QProgressBar*>(cellWidget(jobRow, ProgressColumn));
    if (progress)
        progress->setValue(done);
}

void ChatTransfersView::slotStateChanged(QXmppTransferJob::State state)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    item(jobRow, NameColumn)->setIcon(jobIcon(job));
    emit updateButtons();
}

ChatTransfers::ChatTransfers(QXmppClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), client(xmppClient)
{
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("File transfers"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);

    /* status bar */
    QLayout *hbox = headerLayout();
    layout->addItem(hbox);

    /* help label */
    QLabel *helpLabel = new QLabel(tr("The file transfer feature is experimental and the transfer speed is limited so as not to interfere with your internet connection."));
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    /* download location label */
    const QString downloadsLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(SystemInfo::storageLocation(SystemInfo::DownloadsLocation)).toString(),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    QLabel *downloadsLabel = new QLabel(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(downloadsLink));
    downloadsLabel->setOpenExternalLinks(true);
    downloadsLabel->setWordWrap(true);
    layout->addWidget(downloadsLabel);

    /* transfers list */
    tableWidget = new ChatTransfersView;
    connect(tableWidget, SIGNAL(updateButtons()), this, SLOT(updateButtons()));
    connect(tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(updateButtons()));
    layout->addWidget(tableWidget);

    /* buttons */
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    removeButton = new QPushButton;
    connect(removeButton, SIGNAL(clicked()), tableWidget, SLOT(removeCurrentJob()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);
    updateButtons();

    /* connect signals */
    connect(&client->getTransferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
        this, SLOT(fileReceived(QXmppTransferJob*)));
}

ChatTransfers::~ChatTransfers()
{
}

int ChatTransfers::activeJobs(QXmppTransferJob::Direction direction) const
{
    return tableWidget->activeJobs(direction);
}

void ChatTransfers::addJob(QXmppTransferJob *job)
{
    tableWidget->addJob(job);
    emit registerPanel();
}

QString ChatTransfers::availableFilePath(const QString &dirPath, const QString &name)
{
    QString fileName = name;
    QDir downloadsDir(dirPath);
    if (downloadsDir.exists(fileName))
    {
        const QString fileBase = QFileInfo(fileName).completeBaseName();
        const QString fileSuffix = QFileInfo(fileName).suffix();
        int i = 2;
        while (downloadsDir.exists(fileName))
        {
            fileName = QString("%1_%2").arg(fileBase, QString::number(i++));
            if (!fileSuffix.isEmpty())
                fileName += "." + fileSuffix;
        }
    }
    return downloadsDir.absoluteFilePath(fileName);
}

void ChatTransfers::fileAccepted(QXmppTransferJob *job)
{
    // determine file location
    QDir downloadsDir(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));
    const QString filePath = availableFilePath(downloadsDir.path(), job->fileName());

    // open file
    QFile *file = new QFile(filePath, job);
    if (!file->open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write to" << filePath;
        job->abort();
        return;
    }

    // add transfer to list
    job->setData(LocalPathRole, filePath);
    addJob(job);

    // start transfer
    job->accept(file);
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

void ChatTransfers::sendFile(const QString &fullJid)
{
    // get file name
    QString filePath = QFileDialog::getOpenFileName(this, tr("Send a file"));
    if (filePath.isEmpty())
        return;

    // check file size
    if (QFileInfo(filePath).size() > fileSizeLimit)
    {
        QMessageBox::warning(this,
            tr("Send a file"),
            tr("Sorry, but you cannot send files bigger than %1.")
                .arg(ChatTransfers::sizeToString(fileSizeLimit)));
        return;
    }

    // send file
    QXmppTransferJob *job = client->getTransferManager().sendFile(fullJid, filePath);
    job->setData(LocalPathRole, filePath);
    addJob(job);
}

QSize ChatTransfers::sizeHint() const
{
    return QSize(400, 200);
}

QString ChatTransfers::sizeToString(qint64 size)
{
    if (size < KILOBYTE)
        return QString::fromUtf8("%1 B").arg(size);
    else if (size < MEGABYTE)
        return QString::fromUtf8("%1 KB").arg(double(size) / double(KILOBYTE), 0, 'f', 1);
    else if (size < GIGABYTE)
        return QString::fromUtf8("%1 MB").arg(double(size) / double(MEGABYTE), 0, 'f', 1);
    else
        return QString::fromUtf8("%1 GB").arg(double(size) / double(GIGABYTE), 0, 'f', 1);
}

void ChatTransfers::updateButtons()
{
    QXmppTransferJob *job = tableWidget->currentJob();
    if (!job)
    {
        removeButton->setEnabled(false);
        removeButton->setIcon(QIcon(":/remove.png"));
    }
    else if (job->state() == QXmppTransferJob::FinishedState)
    {
        removeButton->setIcon(QIcon(":/remove.png"));
        removeButton->setEnabled(true);
    }
    else
    {
        removeButton->setIcon(QIcon(":/close.png"));
        removeButton->setEnabled(true);
    }
}

