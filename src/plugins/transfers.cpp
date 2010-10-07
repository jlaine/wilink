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
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

#include "QXmppClient.h"
#include "QXmppShareExtension.h"
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_panel.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "systeminfo.h"
#include "transfers.h"
#include "chat_utils.h"

#define TRANSFERS_ROSTER_ID    "0_transfers"

static qint64 fileSizeLimit = 50000000; // 50 MB

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
            .arg(contactName, job->fileName(), sizeToString(job->fileSize())));
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
    if (standardButton(button) != QMessageBox::Yes)
    {
        m_job->abort();
        return;
    }

    // determine file location
    QDir downloadsDir(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));
    const QString filePath = QXmppShareExtension::availableFilePath(downloadsDir.path(), m_job->fileName());

    // open file
    QFile *file = new QFile(filePath, m_job);
    if (!file->open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write to" << filePath;
        m_job->abort();
        return;
    }

    // start transfer
    m_job->setData(QXmppShareExtension::LocalPathRole, filePath);
    m_job->accept(file);

    emit fileAccepted(m_job);
}

ChatTransferWidget::ChatTransferWidget(QXmppTransferJob *job, QWidget *parent)
    : QWidget(parent),
    m_job(job)
{
    QHBoxLayout *layout = new QHBoxLayout;
    m_icon = new QLabel;
    if (m_job->direction() == QXmppTransferJob::IncomingDirection)
        m_icon->setPixmap(QPixmap(":/download.png"));
    else
        m_icon->setPixmap(QPixmap(":/upload.png"));
    layout->addWidget(m_icon);

    layout->addWidget(new QLabel(QString("%1 (%2)").arg(
        m_job->fileName(),
        sizeToString(job->fileSize()))));

    m_progress = new QProgressBar;
    layout->addWidget(m_progress);

    m_cancelButton = new QPushButton;
    m_cancelButton->setIcon(QIcon(":/close.png"));
    m_cancelButton->setFlat(true);
    m_cancelButton->setMaximumWidth(32);
    layout->addWidget(m_cancelButton);

    setLayout(layout);

    // connect signals
    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(slotCancel()));
    connect(m_job, SIGNAL(destroyed(QObject*)),
            this, SLOT(slotDestroyed(QObject*)));
    connect(m_job, SIGNAL(progress(qint64, qint64)),
            this, SLOT(slotProgress(qint64, qint64)));
    connect(m_job, SIGNAL(stateChanged(QXmppTransferJob::State)),
            this, SLOT(slotStateChanged(QXmppTransferJob::State)));
}

void ChatTransferWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_job)
        return;

    const QString localFilePath = m_job->data(QXmppShareExtension::LocalPathRole).toString();
    if (localFilePath.isEmpty())
        return;
    if (m_job->direction() == QXmppTransferJob::IncomingDirection &&
        (m_job->state() != QXmppTransferJob::FinishedState ||
         m_job->error() != QXmppTransferJob::NoError))
        return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath));
}

void ChatTransferWidget::slotCancel()
{
    // cancel job
    if (m_job && m_job->state() != QXmppTransferJob::FinishedState)
    {
        m_job->abort();
        return;
    }

    // delete job and widget
    if (m_job)
        m_job->deleteLater();
    deleteLater();
}

void ChatTransferWidget::slotDestroyed(QObject *object)
{
    Q_UNUSED(object);
    m_job = 0;
    m_cancelButton->hide();
}

void ChatTransferWidget::slotProgress(qint64 done, qint64 total)
{
    if (m_job && total > 0)
    {
        m_progress->setValue((100 * done) / total);
        qint64 speed = m_job->speed();
        if (m_job->direction() == QXmppTransferJob::IncomingDirection)
            m_progress->setToolTip(tr("Downloading at %1").arg(speedToString(speed)));
        else
            m_progress->setToolTip(tr("Uploading at %1").arg(speedToString(speed)));
    }
}

void ChatTransferWidget::slotStateChanged(QXmppTransferJob::State state)
{
    if (state == QXmppTransferJob::FinishedState)
    {
        m_cancelButton->setIcon(QIcon(":/remove.png"));
        m_progress->hide();
        if (m_job->error() == QXmppTransferJob::NoError)
            m_icon->setPixmap(QPixmap(":/contact-available.png"));
        else
            m_icon->setPixmap(QPixmap(":/contact-busy.png"));
    }
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

void ChatTransfersView::addJob(QXmppTransferJob *job)
{
    if (jobs.contains(job))
        return;

    const QString fileName = QFileInfo(job->data(QXmppShareExtension::LocalPathRole).toString()).fileName();

    jobs.insert(0, job);
    insertRow(0);
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileName);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    nameItem->setIcon(jobIcon(job));
    setItem(0, NameColumn, nameItem);

    QTableWidgetItem *sizeItem = new QTableWidgetItem(sizeToString(job->fileSize()));
    sizeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    setItem(0, SizeColumn, sizeItem);

    QProgressBar *progress = new QProgressBar;
    progress->setMaximum(100);
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
    Q_UNUSED(column);

    if (row < 0 || row >= jobs.size())
        return;

    QXmppTransferJob *job = jobs.at(row);
    const QString localFilePath = job->data(QXmppShareExtension::LocalPathRole).toString();
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
    const QString localFilePath = job->data(QXmppShareExtension::LocalPathRole).toString();
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
    if (progress && total > 0)
    {
        progress->setValue((100 * done) / total);
        qint64 speed = job->speed();
        if (job->direction() == QXmppTransferJob::IncomingDirection)
            progress->setToolTip(tr("Downloading at %1").arg(speedToString(speed)));
        else
            progress->setToolTip(tr("Uploading at %1").arg(speedToString(speed)));
    }
}

void ChatTransfersView::slotStateChanged(QXmppTransferJob::State state)
{
    Q_UNUSED(state);

    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    int jobRow = jobs.indexOf(job);
    if (!job || jobRow < 0)
        return;

    item(jobRow, NameColumn)->setIcon(jobIcon(job));
    emit updateButtons();
}

ChatTransfersWatcher::ChatTransfersWatcher(Chat *window)
    : QObject(window),
    chatWindow(window)
{
    // disable in-band bytestreams
    chatWindow->client()->transferManager().setSupportedMethods(
        QXmppTransferJob::SocksMethod);

#if 0
    const QString downloadsLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(SystemInfo::storageLocation(SystemInfo::DownloadsLocation)).toString(),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    setWindowHelp(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(downloadsLink));
#endif

    /* connect signals */
    connect(&chatWindow->client()->transferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
        this, SLOT(fileReceived(QXmppTransferJob*)));
}

void ChatTransfersWatcher::addJob(QXmppTransferJob *job)
{
    ChatTransferWidget *widget = new ChatTransferWidget(job);
    const QString bareJid = jidToBareJid(job->jid());
    QModelIndex index = chatWindow->rosterModel()->findItem(bareJid);
    if (index.isValid())
        QMetaObject::invokeMethod(chatWindow, "rosterClicked", Q_ARG(QModelIndex, index));

    ChatPanel *panel = chatWindow->panel(bareJid);
    if (panel)
        panel->addWidget(widget);
}

void ChatTransfersWatcher::fileReceived(QXmppTransferJob *job)
{
    // if the job was already accepted or refused (by the shares plugin)
    // stop here
    if (job->state() != QXmppTransferJob::OfferState)
        return;

    const QString bareJid = jidToBareJid(job->jid());
//    const QString contactName = rosterModel->contactName(bareJid);

    // prompt user
    ChatTransferPrompt *dlg = new ChatTransferPrompt(job, bareJid, chatWindow);
    connect(dlg, SIGNAL(fileAccepted(QXmppTransferJob*)), this, SLOT(addJob(QXmppTransferJob*)));
    dlg->show();
}

/** Handle file drag & drop on roster entries.
 */
void ChatTransfersWatcher::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    const QStringList fullJids = chatWindow->rosterModel()->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
    if (!chatWindow->client()->isConnected() ||
        !event->mimeData()->hasUrls() ||
        fullJids.isEmpty())
        return;

    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        if (url.scheme() != "file")
            continue;
        if (event->type() == QEvent::Drop)
            sendFile(fullJids.first(), url.toLocalFile());
        found++;
    }
    if (found)
        event->acceptProposedAction();
}

void ChatTransfersWatcher::rosterMenu(QMenu *menu, const QModelIndex &index)
{
    const QString jid = index.data(ChatRosterModel::IdRole).toString();
    const QStringList fullJids = chatWindow->rosterModel()->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
    if (!chatWindow->client()->isConnected() ||
        fullJids.isEmpty())
        return;

    QAction *action = menu->addAction(QIcon(":/add.png"), tr("Send a file"));
    action->setData(fullJids.first());
    connect(action, SIGNAL(triggered()), this, SLOT(sendFilePrompt()));
}

void ChatTransfersWatcher::sendFile(const QString &fullJid, const QString &filePath)
{
    // check file size
    if (QFileInfo(filePath).size() > fileSizeLimit)
    {
        QMessageBox::warning(chatWindow,
            tr("Send a file"),
            tr("Sorry, but you cannot send files bigger than %1.")
                .arg(sizeToString(fileSizeLimit)));
        return;
    }

    // send file
    QXmppTransferJob *job = chatWindow->client()->transferManager().sendFile(fullJid, filePath);
    job->setData(QXmppShareExtension::LocalPathRole, filePath);
    addJob(job);
}

void ChatTransfersWatcher::sendFileAccepted(const QString &filePath)
{
    QString fullJid = sender()->objectName();
    sendFile(fullJid, filePath);
}

void ChatTransfersWatcher::sendFilePrompt()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    QString fullJid = action->data().toString();

    QFileDialog *dialog = new QFileDialog(chatWindow, tr("Send a file"));
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setObjectName(fullJid);
    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(deleteLater()));
    dialog->open(this, SLOT(sendFileAccepted(QString)));
}

// PLUGIN

class TransfersPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool TransfersPlugin::initialize(Chat *chat)
{
    /* register panel */
    ChatTransfersWatcher *transfers = new ChatTransfersWatcher(chat);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
            transfers, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    connect(chat, SIGNAL(rosterMenu(QMenu*, QModelIndex)),
            transfers, SLOT(rosterMenu(QMenu*, QModelIndex)));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(transfers, TransfersPlugin)

