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
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppUtils.h"
#include "qxmpp/QXmppShareIq.h"

#include "chat_shares.h"
#include "chat_transfers.h"
#include "systeminfo.h"

#define KILOBYTE 1000
#define MEGABYTE 1000000
#define GIGABYTE 1000000000

static qint64 fileSizeLimit = 10000000; // 10 MB
static int parallelDownloadLimit = 2;

enum TransfersColumns {
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

enum DataRoles {
    PacketId,
    StreamId,
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

ChatTransfers::ChatTransfers(QXmppClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), client(xmppClient)
{
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("File transfers"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);

    /* status bar */
    QLayout *hbox = statusBar();
    layout->addItem(hbox);

    /* help label */
    QLabel *helpLabel = new QLabel(tr("The file transfer feature is experimental and the transfer speed is limited so as not to interfere with your internet connection."));
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    /* download location label */
    const QString downloadsLink = QString("<a href=\"file://%1\">%2</a>").arg(
        SystemInfo::storageLocation(SystemInfo::DownloadsLocation),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    QLabel *downloadsLabel = new QLabel(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.")
        .arg(downloadsLink));
    downloadsLabel->setOpenExternalLinks(true);
    downloadsLabel->setWordWrap(true);
    layout->addWidget(downloadsLabel);

    /* transfers list */
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

    /* download queue */
    queueModel = new ChatSharesModel(this);
    queueView = new ChatSharesView;
    queueView->setModel(queueModel);
    //queueView->hide();
    layout->addWidget(queueView);


    /* buttons */
    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    removeButton = new QPushButton;
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeCurrentJob()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);
    updateButtons();

    /* connect signals */
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(&client->getTransferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
        this, SLOT(fileReceived(QXmppTransferJob*)));
}

ChatTransfers::~ChatTransfers()
{
}

void ChatTransfers::addJob(QXmppTransferJob *job)
{
    if (jobs.contains(job))
        return;

    const QString fileName = QFileInfo(job->data(LocalPathRole).toString()).fileName();

    jobs.insert(0, job);
    tableWidget->insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileName);
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

    emit registerTab();
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
    processDownloadQueue();

    // find the job that completed
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
    QStringList pathBits;
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, StreamId, job->sid());
    if (queueItem)
    {
        // find full path
        QXmppShareItem *parentItem = queueItem->parent();
        while (parentItem && parentItem->parent())
        {
            // sanitize path
            QString dirName = parentItem->name();
            if (dirName != "." && dirName != ".." && !dirName.contains("/") && !dirName.contains("\\"))
                pathBits.prepend(dirName);
            parentItem = parentItem->parent();
        }

        // remove from queue
        queueModel->removeItem(queueItem);
        queueModel->pruneEmptyChildren();
    }

    // determine file location
    QDir downloadsDir(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));
    if (pathBits.size() > 0)
    {
        QString subdir = pathBits.join("/");
        if (downloadsDir.exists(subdir) || downloadsDir.mkpath(subdir))
            downloadsDir.setPath(downloadsDir.filePath(subdir));
    }

    QString fileName = job->fileName();
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

    const QString filePath = downloadsDir.absoluteFilePath(fileName);
    QFile *file = new QFile(filePath, job);
    if (file->open(QIODevice::WriteOnly))
    {
        job->setData(LocalPathRole, filePath);

        // add transfer to list
        addJob(job);

        // start transfer
        job->accept(file);
    } else {
        qWarning() << "Could not write to" << filePath;
        job->abort();
    }
}

void ChatTransfers::fileDeclined(QXmppTransferJob *job)
{
    job->abort();
}

void ChatTransfers::fileReceived(QXmppTransferJob *job)
{
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, StreamId, job->sid());
    if (queueItem)
    {
        fileAccepted(job);
        return;
    }
    const QString bareJid = jidToBareJid(job->jid());
//    const QString contactName = rosterModel->contactName(bareJid);

    // prompt user
    ChatTransferPrompt *dlg = new ChatTransferPrompt(job, bareJid, this);
    connect(dlg, SIGNAL(fileAccepted(QXmppTransferJob*)), this, SLOT(fileAccepted(QXmppTransferJob*)));
    connect(dlg, SIGNAL(fileDeclined(QXmppTransferJob*)), this, SLOT(fileDeclined(QXmppTransferJob*)));
    dlg->show();
}

void ChatTransfers::getFile(const QXmppShareItem &file)
{
    queueModel->addItem(file);
    queueModel->pruneEmptyChildren();
    processDownloadQueue();
}

void ChatTransfers::processDownloadQueue()
{
    // check how many downloads are active
    int activeDownloads = 0;
    for (int i = 0; i < jobs.size(); i++)
        if (jobs[i]->direction() == QXmppTransferJob::IncomingDirection &&
            jobs[i]->state() != QXmppTransferJob::FinishedState)
            activeDownloads++;

    while (activeDownloads < parallelDownloadLimit)
    {
        // find next item
        QXmppShareItem *file = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, QVariant());
        if (!file)
            return;

        // pick mirror
        QXmppShareMirror mirror;
        bool mirrorFound = false;
        foreach (mirror, file->mirrors())
        {
            if (!mirror.jid().isEmpty() && !mirror.path().isEmpty())
            {
                mirrorFound = true;
                break;
            }
        }
        if (!mirrorFound)
        {
            qWarning() << "No mirror found for file" << file->name();
            return;
        }

        // request file
        QXmppShareGetIq iq;
        iq.setTo(mirror.jid());
        iq.setType(QXmppIq::Get);
        iq.setFile(*file);
        qDebug() << "Requesting file" << iq.file().name() << "from" << iq.to();
        file->setData(PacketId, iq.id());
        client->sendPacket(iq);

        activeDownloads++;
    }
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
        job->deleteLater();
    } else {
        jobs.at(jobRow)->abort();
    }
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

void ChatTransfers::shareGetIqReceived(const QXmppShareGetIq &shareIq)
{
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, shareIq.id());
    if (!queueItem)
        return;

    if (shareIq.type() == QXmppIq::Result)
    {
        // expect file
        qDebug() << "Expecting file" << shareIq.file().name() << "from" << shareIq.from();
        queueItem->setData(StreamId, shareIq.sid());
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        qWarning() << "Error requesting file" << shareIq.file().name() << "from" << shareIq.from();
    }
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

