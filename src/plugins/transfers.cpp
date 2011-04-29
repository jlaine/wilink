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

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QProgressBar>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>

#include "QXmppClient.h"
#include "QXmppShareExtension.h"
#include "QXmppUtils.h"

#include "chat.h"
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
    const QString filePath = availableFilePath(downloadsDir.path(), m_job->fileName());

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

ChatTransferWidget::ChatTransferWidget(QXmppTransferJob *job, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
    m_job(job)
{
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal, this);
    layout->setContentsMargins(0, 0, 0, 0);

    // icon
    m_icon = new ChatPanelImage(this);
    if (m_job->direction() == QXmppTransferJob::IncomingDirection)
    {
        m_icon->setPixmap(QPixmap(":/download.png"));
        m_disappearWhenFinished = false;
    } else {
        m_icon->setPixmap(QPixmap(":/upload.png"));
        m_disappearWhenFinished = true;
    }
    layout->addItem(m_icon);

    // progress bar
    m_progress = new ChatPanelProgress(this);
    layout->addItem(m_progress);

    // status label
    m_label = new ChatPanelText(QString("%1 (%2)").arg(
        m_job->fileName(),
        sizeToString(job->fileSize())), this);
    layout->addItem(m_label);

    // close button
    m_button = new ChatPanelButton(this);
    m_button->setPixmap(QPixmap(":/close.png"));
    layout->addItem(m_button);

    setLayout(layout);

    // connect signals
    bool check;
    check = connect(m_button, SIGNAL(clicked()),
                    this, SLOT(slotCancel()));
    Q_ASSERT(check);

    check = connect(m_label, SIGNAL(clicked()),
                    this, SLOT(slotOpen()));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(destroyed(QObject*)),
                    this, SLOT(slotDestroyed(QObject*)));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(finished()),
                    this, SLOT(slotFinished()));
    Q_ASSERT(check);

    check = connect(m_job, SIGNAL(progress(qint64, qint64)),
                    this, SLOT(slotProgress(qint64, qint64)));
    Q_ASSERT(check);
}

void ChatTransferWidget::slotCancel()
{
    if (m_job && m_job->state() != QXmppTransferJob::FinishedState)
    {
        // cancel job
        m_disappearWhenFinished = true;
        m_job->abort();
        return;
    } else {
        // make widget disappear
        m_button->setEnabled(false);
        emit finished();
    }
}

void ChatTransferWidget::slotDestroyed(QObject *object)
{
    Q_UNUSED(object);
    m_job = 0;
}

void ChatTransferWidget::slotOpen()
{
    if (!m_localPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_localPath));
}

void ChatTransferWidget::slotProgress(qint64 done, qint64 total)
{
    if (m_job && total > 0)
    {
        m_progress->setValue((100 * done) / total);
        qint64 speed = m_job->speed();
        if (m_job->direction() == QXmppTransferJob::IncomingDirection)
            setToolTip(tr("Downloading at %1").arg(speedToString(speed)));
        else
            setToolTip(tr("Uploading at %1").arg(speedToString(speed)));
    }
}

void ChatTransferWidget::slotFinished()
{
    // update UI
//    m_progress->hide();
    setToolTip(QString());
    if (m_job->error() == QXmppTransferJob::NoError)
    {
        m_icon->setPixmap(QPixmap(":/contact-available.png"));
        m_localPath = m_job->data(QXmppShareExtension::LocalPathRole).toString();
    }
    else
        m_icon->setPixmap(QPixmap(":/contact-busy.png"));

    // delete job
    m_job->deleteLater();

    // make widget disappear
    if (m_disappearWhenFinished)
    {
        m_button->setEnabled(false);
        QTimer::singleShot(1000, this, SIGNAL(finished()));
    }
}

ChatTransfersWatcher::ChatTransfersWatcher(Chat *window)
    : QObject(window),
    chatWindow(window)
{
    transferManager = chatWindow->client()->findExtension<QXmppTransferManager>();
    if (!transferManager) {
        transferManager = new QXmppTransferManager;
        chatWindow->client()->addExtension(transferManager);
    }
    // disable in-band bytestreams
    transferManager->setSupportedMethods(QXmppTransferJob::SocksMethod);

#if 0
    const QString downloadsLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(SystemInfo::storageLocation(SystemInfo::DownloadsLocation)).toString(),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    setWindowHelp(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(downloadsLink));
#endif

    /* connect signals */
    connect(transferManager, SIGNAL(fileReceived(QXmppTransferJob*)),
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

    // prompt user
    const QString contactName = chatWindow->rosterModel()->contactName(job->jid());
    ChatTransferPrompt *dlg = new ChatTransferPrompt(job, contactName, chatWindow);
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
    QXmppTransferJob *job = transferManager->sendFile(fullJid, filePath);
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
    void finalize(Chat *chat);
    bool initialize(Chat *chat);
    QString name() const { return "File transfers"; };
    void polish(Chat *chat, ChatPanel *panel);

private:
    QMap<Chat*,ChatTransfersWatcher*> m_watchers;
};

bool TransfersPlugin::initialize(Chat *chat)
{
    /* register panel */
    ChatTransfersWatcher *watcher = new ChatTransfersWatcher(chat);

    /* add roster hooks */
    connect(chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
            watcher, SLOT(rosterDrop(QDropEvent*, QModelIndex)));

    m_watchers.insert(chat, watcher);
    return true;
}

void TransfersPlugin::finalize(Chat *chat)
{
    m_watchers.remove(chat);
}

void TransfersPlugin::polish(Chat *chat, ChatPanel *panel)
{
    ChatTransfersWatcher *watcher = m_watchers.value(chat);
    if (!watcher || panel->objectType() != ChatRosterModel::Contact)
        return;

    const QStringList fullJids = chat->rosterModel()->contactFeaturing(panel->objectName(), ChatRosterModel::FileTransferFeature);
    if (!fullJids.isEmpty()) {
        QAction *action = panel->addAction(QIcon(":/add.png"), QObject::tr("Send a file"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), watcher, SLOT(sendFilePrompt()));
    }
}

Q_EXPORT_STATIC_PLUGIN2(transfers, TransfersPlugin)

