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

#include <QDir>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QStringList>
#include <QTimer>
#include <QTreeWidget>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_shares.h"
#include "chat_shares_database.h"
#include "chat_transfers.h"

enum Columns
{
    NameColumn,
    SizeColumn,
    MaxColumn,
};

enum Roles
{
    HashRole = Qt::UserRole,
    SizeRole,
    TypeRole,
    MirrorRole,
    PathRole,
};

enum Types
{
    CollectionType = 0,
    FileType = 1,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), client(xmppClient), db(0)
{
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("Shares"));

    qRegisterMetaType<QXmppShareSearchIq>("QXmppShareSearchIq");
    sharesDir = QDir(QDir::home().filePath("Public"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addItem(statusBar());
    layout->addWidget(new QLabel(tr("Enter the name of the file you are looking for.")));
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    layout->addWidget(lineEdit);

    treeWidget = new ChatSharesView;
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*)));
    layout->addWidget(treeWidget);

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
}

void ChatShares::goBack()
{
}

void ChatShares::shareGetIqReceived(const QXmppShareGetIq &shareIq)
{
#if 0
    if (shareIq.from() != shareServer)
        return;
#endif

    if (shareIq.type() == QXmppIq::Get)
    {
        QXmppShareGetIq responseIq;
        responseIq.setId(shareIq.id());
        responseIq.setTo(shareIq.from());
        responseIq.setType(QXmppIq::Result);

        // check path is OK
        QString filePath = db->locate(shareIq.file());
        if (filePath.isEmpty())
        {
            qWarning() << "Could not find file" << shareIq.file().name() << shareIq.file().fileSize();
            responseIq.setType(QXmppIq::Error);
            client->sendPacket(responseIq);
            return;
        }
        responseIq.setSid(generateStanzaHash());
        client->sendPacket(responseIq);

        // send files
        qDebug() << "Sending" << shareIq.file().name() << "to" << shareIq.from();
        QXmppTransferJob *job = client->getTransferManager().sendFile(shareIq.from(), filePath, responseIq.sid());
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    }
    else if (shareIq.type() == QXmppIq::Result)
    {
        // expect file
        qDebug() << "Expecting file transfer" << shareIq.sid();
        emit fileExpected(shareIq.sid());
    }
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
    {
        db->search(shareIq);
        return;
    }

    lineEdit->setEnabled(true);
    if (shareIq.type() == QXmppIq::Result)
    {
        QTreeWidgetItem *parent = treeWidget->findItem(shareIq.collection(), 0);
        if (!parent)
            treeWidget->clear();
        foreach (const QXmppShareIq::Item &item, shareIq.collection().children())
            treeWidget->addItem(item, parent);
        if (parent)
            parent->setExpanded(true);
    }
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();

    lineEdit->setEnabled(false);

    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
}

void ChatShares::itemDoubleClicked(QTreeWidgetItem *item)
{
    const QString jid = item->data(NameColumn, MirrorRole).toString();
    const QString path = item->data(NameColumn, PathRole).toString();
    if (jid.isEmpty())
    {
        qWarning() << "No mirror for item" << item->text(NameColumn);
        return; 
    }

    int type = item->data(NameColumn, TypeRole).toInt();
    if (type == FileType)
    {
        if (path.isEmpty())
        {
            qWarning() << "No path for file" << item->text(NameColumn);
            return;
        }

        // request file
        QXmppShareGetIq iq;
        iq.setTo(jid);
        iq.setType(QXmppIq::Get);
        iq.file().setName(item->text(NameColumn));
        iq.file().setFileHash(item->data(NameColumn, HashRole).toByteArray());
        iq.file().setFileSize(item->data(NameColumn, SizeRole).toInt());
        qDebug() << "Requesting" << iq.file().name() << "from" << iq.to();
        client->sendPacket(iq);
    }
    else if (type == CollectionType && !item->childCount())
    {
        lineEdit->setEnabled(false);

        // browse files
        QXmppShareSearchIq iq;
        iq.setTo(jid);
        iq.setType(QXmppIq::Get);
        iq.setBase(path);
        client->sendPacket(iq);
    }
}

void ChatShares::registerWithServer()
{
    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(x);
    client->sendPacket(presence);
}

void ChatShares::setShareServer(const QString &server)
{
    shareServer = server;

    if (!db)
    {
        db = new ChatSharesDatabase(sharesDir.path(), this);
        connect(db, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SLOT(searchFinished(const QXmppShareSearchIq&)));
    }

    // register with server
    registerWithServer();
    registerTimer->start();

    // browse peers
    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    client->sendPacket(iq);
}

int ChatSharesModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatSharesModel::data(const QModelIndex &index, int role) const
{

}

QModelIndex ChatSharesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmppShareIq::Item *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareIq::Item*>(parent.internalPointer());

    QXmppShareIq::Item *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ChatSharesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareIq::Item *childItem = static_cast<QXmppShareIq::Item*>(index.internalPointer());
    QXmppShareIq::Item *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ChatSharesModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareIq::Item *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareIq::Item*>(parent.internalPointer());
    return parentItem->size();
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeWidget(parent)
{
    ChatSharesModel *model = new ChatSharesModel;

    setColumnCount(MaxColumn);
    setColumnWidth(SizeColumn, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);
    peerIcon = iconProvider.icon(QFileIconProvider::Network);

    /* set header names */
    clear();
}

qint64 ChatSharesView::addItem(const QXmppShareIq::Item &file, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *fileItem = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);
    fileItem->setData(NameColumn, HashRole, file.fileHash());
    fileItem->setData(NameColumn, SizeRole, file.fileSize());

    /* FIXME : we are only using the first mirror */
    bool isPeer = false;
    if (!file.mirrors().isEmpty())
    {
        QXmppShareIq::Mirror mirror = file.mirrors().first();
        fileItem->setData(NameColumn, MirrorRole, mirror.jid());
        fileItem->setData(NameColumn, PathRole, mirror.path());
        if (mirror.path().isEmpty())
            isPeer = true;
    }

    fileItem->setText(NameColumn, file.name());
    if (file.type() == QXmppShareIq::Item::CollectionItem)
    {
        fileItem->setData(NameColumn, TypeRole, CollectionType);
        fileItem->setIcon(NameColumn, isPeer ? peerIcon : collectionIcon);
    } else {
        fileItem->setData(NameColumn, TypeRole, FileType);
        fileItem->setIcon(NameColumn, fileIcon);
    }

    /* calculate size */
    qint64 collectionSize = file.fileSize();
    foreach (const QXmppShareIq::Item &child, file.children())
        collectionSize += addItem(child, fileItem);
    fileItem->setText(SizeColumn, ChatTransfers::sizeToString(collectionSize));
    return collectionSize;
}

void ChatSharesView::clear()
{
    QTreeWidget::clear();
    QTreeWidgetItem *headerItem = new QTreeWidgetItem;
    headerItem->setText(NameColumn, tr("Name"));
    headerItem->setText(SizeColumn, tr("Size"));
    setHeaderItem(headerItem);
}

QTreeWidgetItem *ChatSharesView::findItem(const QXmppShareIq::Item &collection, QTreeWidgetItem *parent)
{
    if (collection.mirrors().isEmpty())
        return 0;

    /* FIXME : we are only using the first mirror */
    QXmppShareIq::Mirror mirror = collection.mirrors().first();
    QTreeWidgetItem *found = 0;
    if (parent)
    {
        if (mirror.jid() == parent->data(NameColumn, MirrorRole) &&
            mirror.path() == parent->data(NameColumn, PathRole))
            return parent;
        for (int i = 0; i < parent->childCount(); i++)
            if ((found = findItem(collection, parent->child(i))) != 0)
                return found;
    } else {
        for (int i = 0; i < topLevelItemCount(); i++)
            if ((found = findItem(collection, topLevelItem(i))) != 0)
                return found;
    }
    return 0;
}

void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeWidget::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - 80);
}

