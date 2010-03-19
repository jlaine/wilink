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
    connect(lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(queryStringChanged()));
    layout->addWidget(lineEdit);

    /* create model / view */
    ChatSharesModel *model = new ChatSharesModel;
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), model, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
    connect(model, SIGNAL(itemReceived(const QModelIndex&)), this, SLOT(itemReceived(const QModelIndex&)));
    treeWidget = new ChatSharesView;
    treeWidget->setModel(model);
    connect(treeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    layout->addWidget(treeWidget);

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
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
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();

    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
}

void ChatShares::itemDoubleClicked(const QModelIndex &index)
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return;

    if (item->mirrors().isEmpty())
    {
        qWarning() << "No mirror for item" << item->name();
        return; 
    }
    const QXmppShareMirror mirror = item->mirrors().first();

    if (item->type() == QXmppShareItem::FileItem)
    {
        if (mirror.path().isEmpty())
        {
            qWarning() << "No path for file" << item->name();
            return;
        }

        // request file
        QXmppShareGetIq iq;
        iq.setTo(mirror.jid());
        iq.setType(QXmppIq::Get);
        iq.setFile(*item);
        qDebug() << "Requesting" << iq.file().name() << "from" << iq.to();
        client->sendPacket(iq);
    }
    else if (item->type() == QXmppShareItem::CollectionItem && !item->size())
    {
        // browse files
        QXmppShareSearchIq iq;
        iq.setTo(mirror.jid());
        iq.setType(QXmppIq::Get);
        iq.setBase(mirror.path());
        client->sendPacket(iq);
    }
}

void ChatShares::itemReceived(const QModelIndex &index)
{
    treeWidget->setExpanded(index, true);
}

void ChatShares::queryStringChanged()
{
    if (lineEdit->text().isEmpty())
        findRemoteFiles();
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
    findRemoteFiles();
}

ChatSharesModel::ChatSharesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new QXmppShareItem(QXmppShareItem::CollectionItem);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);
    peerIcon = iconProvider.icon(QFileIconProvider::Network);
}

int ChatSharesModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatSharesModel::data(const QModelIndex &index, int role) const
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == NameColumn)
        return item->name();
    else if (role == Qt::DisplayRole && index.column() == SizeColumn && item->fileSize())
        return ChatTransfers::sizeToString(item->fileSize());
    else if (role == Qt::DecorationRole && index.column() == NameColumn)
    {
        if (item->type() == QXmppShareItem::CollectionItem)
        {
            if (item->mirrors().size() == 1 && item->mirrors().first().path().isEmpty())
                return peerIcon;
            else
                return collectionIcon;
        }
        else
            return fileIcon;
    }
    return QVariant();
}

QVariant ChatSharesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == NameColumn)
            return tr("Name");
        else if (section == SizeColumn)
            return tr("Size");
    }
    return QVariant();
}

QModelIndex ChatSharesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());

    QXmppShareItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ChatSharesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareItem *childItem = static_cast<QXmppShareItem*>(index.internalPointer());
    QXmppShareItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ChatSharesModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());
    return parentItem->size();
}

void ChatSharesModel::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Result)
    {
        QXmppShareItem *parentItem = rootItem->findChild(shareIq.collection().mirrors());
        if (!parentItem)
        {
            rootItem->clearChildren();
            foreach (const QXmppShareItem &child, shareIq.collection().children())
                rootItem->appendChild(child);
            emit reset();
        } else {
            if (parentItem->size())
            {
                beginRemoveRows(createIndex(parentItem->row(), 0, parentItem), 0, parentItem->size());
                parentItem->clearChildren();
                endRemoveRows();
            }
            if (shareIq.collection().size())
            {
                beginInsertRows(createIndex(parentItem->row(), 0, parentItem), 0, shareIq.collection().size());
                foreach (const QXmppShareItem &child, shareIq.collection().children())
                    parentItem->appendChild(child);
                endInsertRows();
            }
            emit itemReceived(createIndex(parentItem->row(), 0, parentItem));
        }
    }
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeView(parent)
{
    setColumnWidth(SizeColumn, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - 80);
}

