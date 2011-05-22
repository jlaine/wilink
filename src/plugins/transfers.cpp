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
#include <QDeclarativeComponent>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
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
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_utils.h"
#include "chats.h"
#include "systeminfo.h"
#include "transfers.h"

static qint64 fileSizeLimit = 50000000; // 50 MB

enum TransfersColumns {
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

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

    // start transfer
    m_job->accept(filePath);
    emit fileAccepted(m_job);
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
    const QString bareJid = jidToBareJid(job->jid());
    QModelIndex index = chatWindow->rosterModel()->findItem(bareJid);
    if (index.isValid())
        QMetaObject::invokeMethod(chatWindow, "rosterClicked", Q_ARG(QModelIndex, index));

    ChatDialogPanel *panel = qobject_cast<ChatDialogPanel*>(chatWindow->panel(bareJid));
    if (panel) {
        // load component if needed
        QDeclarativeComponent *component = qobject_cast<QDeclarativeComponent*>(panel->property("__transfer_component").value<QObject*>());
        if (!component) {
            QDeclarativeEngine *engine = panel->declarativeView()->engine();
            component = new QDeclarativeComponent(engine, QUrl("qrc:/TransferWidget.qml"));
            panel->setProperty("__transfer_component", qVariantFromValue<QObject*>(component));
        }

        // create transfer widget
        QDeclarativeItem *widget = qobject_cast<QDeclarativeItem*>(component->create());
        Q_ASSERT(widget);
        widget->setProperty("job", qVariantFromValue<QObject*>(job));
        QDeclarativeItem *bar = panel->declarativeView()->rootObject()->findChild<QDeclarativeItem*>("widgetBar");
        widget->setParentItem(bar);
    }
}

/** Handle file drag & drop on conversations.
 */
bool ChatTransfersWatcher::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);

    if (e->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *event = static_cast<QDragEnterEvent*>(e);
        event->acceptProposedAction();
        return true;
    }
    else if (e->type() == QEvent::DragLeave)
    {
        return true;
    }
    else if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop)
    {
        QDropEvent *event = static_cast<QDropEvent*>(e);
        const QString jid = obj->property("__transfer_jid").toString();
        const QStringList fullJids = chatWindow->rosterModel()->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
        int found = 0;
        if (chatWindow->client()->isConnected() &&
            event->mimeData()->hasUrls() &&
            !fullJids.isEmpty())
        {
            foreach (const QUrl &url, event->mimeData()->urls()) {
                if (url.scheme() != "file")
                    continue;
                if (event->type() == QEvent::Drop)
                    sendFile(fullJids.first(), url.toLocalFile());
                found++;
            }
        }
        if (found)
            event->acceptProposedAction();
        return true;
    }
    return false;
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

void TransfersPlugin::finalize(Chat *chat)
{
    m_watchers.remove(chat);
}

bool TransfersPlugin::initialize(Chat *chat)
{
    /* register panel */
    ChatTransfersWatcher *watcher = new ChatTransfersWatcher(chat);
    m_watchers.insert(chat, watcher);
    return true;
}

void TransfersPlugin::polish(Chat *chat, ChatPanel *panel)
{
    ChatTransfersWatcher *watcher = m_watchers.value(chat);
    ChatDialogPanel *dialog = qobject_cast<ChatDialogPanel*>(panel);
    if (!watcher || !dialog)
        return;

    // add action
    const QStringList fullJids = chat->rosterModel()->contactFeaturing(panel->objectName(), ChatRosterModel::FileTransferFeature);
    if (!fullJids.isEmpty()) {
        QAction *action = panel->addAction(QIcon(":/upload.png"), QObject::tr("Send a file"));
        action->setData(fullJids.first());
        connect(action, SIGNAL(triggered()), watcher, SLOT(sendFilePrompt()));
    }

    // handle drag & drop
    QWidget *viewport = dialog->declarativeView()->viewport();
    viewport->setAcceptDrops(true);
    viewport->setProperty("__transfer_jid", dialog->objectName());
    viewport->installEventFilter(watcher);
}

Q_EXPORT_STATIC_PLUGIN2(transfers, TransfersPlugin)

