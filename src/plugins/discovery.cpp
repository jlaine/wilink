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

#include <QAction>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>

#include "QXmppClient.h"
#include "QXmppDiscoveryManager.h"

#include "chat.h"
#include "chat_plugin.h"
#include "declarative.h"
#include "discovery.h"

#define DISCOVERY_ROSTER_ID "0_discovery"

enum Roles
{
    JidRole = Qt::UserRole,
    NodeRole,
    NameRole,
};

static QString itemText(QListWidgetItem *item)
{
    const QString jid = item->data(JidRole).toString();
    const QString node = item->data(NodeRole).toString();
    const QString name = item->data(NameRole).toString();

    QString text;
    if (!name.isEmpty())
        text += "<b>" + name + "</b><br/>";
    text += jid;
    if (!node.isEmpty())
        text += QString(" (%1)").arg(node);
    return text;
}

DiscoveryModel::DiscoveryModel(QObject *parent)
    : ChatModel(parent),
    m_manager(0)
{
}

QVariant DiscoveryModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

void DiscoveryModel::itemsReceived(const QXmppDiscoveryIq &disco)
{

}

QXmppDiscoveryManager *DiscoveryModel::manager() const
{
    return m_manager;
}

void DiscoveryModel::setManager(QXmppDiscoveryManager *manager)
{
    bool check;

    if (manager == manager)
        return;

    m_manager = manager;
    if (m_manager) {
        check = connect(m_manager, SIGNAL(itemsReceived(QXmppDiscoveryIq)),
            this, SLOT(itemsReceived(QXmppDiscoveryIq)));
        Q_ASSERT(check);
    }
    emit managerChanged(m_manager);
}

/** Constructs a DiscoveryPanel.
 *
 * @param client The XMPP client.
 * @param parent The parent widget of the panel.
 */
DiscoveryPanel::DiscoveryPanel(Chat *chatWindow, QXmppClient *client, QWidget *parent)
    : ChatPanel(parent),
    m_client(client)
{
    bool check;

    m_manager = client->findExtension<QXmppDiscoveryManager>();

    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(headerLayout());

    /* location bar */
    QHBoxLayout *hbox = new QHBoxLayout;

    m_locationJid = new QLineEdit;
    hbox->addWidget(m_locationJid);

    m_locationNode = new QLineEdit;
    hbox->addWidget(m_locationNode);

    layout->addLayout(hbox);

    QDeclarativeView *declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("client", new QXmppDeclarativeClient(client));
    context->setContextProperty("window", chatWindow);

    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/DiscoveryPanel.qml"));
    layout->addWidget(declarativeView);

    /* main view */
    m_listWidget = new QListWidget;
    m_listWidget->setIconSize(QSize(32, 32));
    layout->addWidget(m_listWidget);

    setLayout(layout);
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Service discovery"));

    // add actions
    m_backAction = addAction(QIcon(":/back.png"), tr("Go back"));
    m_backAction->setEnabled(false);
    m_backAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    check = connect(m_backAction, SIGNAL(triggered()),
                    this, SLOT(goBack()));
    Q_ASSERT(check);

    QAction *m_refreshAction = addAction(QIcon(":/refresh.png"), tr("Refresh"));
    check = connect(m_refreshAction, SIGNAL(triggered()),
                    this, SLOT(goTo()));
    Q_ASSERT(check);

    /* connect signals */
    check = connect(this, SIGNAL(showPanel()),
        this, SLOT(slotShow()));
    Q_ASSERT(check);

    check = connect(m_manager, SIGNAL(infoReceived(QXmppDiscoveryIq)),
        this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(m_manager, SIGNAL(itemsReceived(QXmppDiscoveryIq)),
        this, SLOT(discoveryItemsReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);


    check = connect(m_locationJid, SIGNAL(returnPressed()),
        this, SLOT(goTo()));

    check = connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(goForward(QListWidgetItem*)));
    Q_ASSERT(check);
}

void DiscoveryPanel::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (!m_requests.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    for (int i = 0; i < m_listWidget->count(); ++i)
    {
        QListWidgetItem *wdgItem = m_listWidget->item(i);
        if (wdgItem->data(JidRole).toString() != disco.from() ||
            wdgItem->data(NodeRole).toString() != disco.queryNode())
            continue;

        if (!disco.identities().isEmpty())
        {
            wdgItem->setData(NameRole, disco.identities().first().name());
            QLabel *label = qobject_cast<QLabel*>(m_listWidget->itemWidget(wdgItem));
            if (label)
                label->setText(itemText(wdgItem));
        }
    }
}

void DiscoveryPanel::discoveryItemsReceived(const QXmppDiscoveryIq &disco)
{
    if (!m_requests.removeAll(disco.id()) || disco.type() != QXmppIq::Result)
        return;

    // check the results are for the current item
    if (m_trail.isEmpty() ||
        m_trail.last().jid() != disco.from() ||
        m_trail.last().node() != disco.queryNode())
        return;

    foreach (const QXmppDiscoveryIq::Item &item, disco.items())
    {
        // insert item
        QListWidgetItem *wdgItem = new QListWidgetItem;
        wdgItem->setIcon(QIcon(":/chat.png"));
        wdgItem->setData(JidRole, item.jid());
        wdgItem->setData(NodeRole, item.node());
        wdgItem->setData(NameRole, item.name());
        QLabel *label = new QLabel(itemText(wdgItem));
        m_listWidget->addItem(wdgItem);
        m_listWidget->setItemWidget(wdgItem, label);

        // request information
        const QString id = m_manager->requestInfo(item.jid(), item.node());
        if (!id.isEmpty())
            m_requests.append(id);
    }
}

void DiscoveryPanel::explore(const QXmppDiscoveryIq::Item &item)
{
    // update window title
    QString extra = item.jid();
    if (!item.node().isEmpty())
        extra += QString(" (%1)").arg(item.node());
    setWindowExtra(extra);
    m_listWidget->clear();

    // update location bar
    m_locationJid->setText(item.jid());
    m_locationNode->setText(item.node());

    // update back button
    if (m_trail.size() < 2)
        m_backAction->setEnabled(false);
    else
        m_backAction->setEnabled(true);

    // request items
    const QString id = m_manager->requestItems(item.jid(), item.node());
    if (!id.isEmpty())
        m_requests.append(id);
}

void DiscoveryPanel::goBack()
{
    if (m_trail.size() < 2)
        return;

    // pop location
    m_trail.pop_back();
    explore(m_trail.last());
}

void DiscoveryPanel::goForward(QListWidgetItem *wdgItem)
{
    QXmppDiscoveryIq::Item item;
    item.setJid(wdgItem->data(JidRole).toString());
    item.setNode(wdgItem->data(NodeRole).toString());
    m_trail.append(item);
    explore(item);
}

void DiscoveryPanel::goTo()
{
    const QString newJid = m_locationJid->text();
    const QString newNode = m_locationNode->text();
    if (newJid.isEmpty())
        return;

    if (!m_trail.isEmpty() &&
        m_trail.last().jid() == newJid &&
        m_trail.last().node() == newNode)
    {
        explore(m_trail.last());
    }
    else
    {
        QXmppDiscoveryIq::Item item;
        item.setJid(newJid);
        item.setNode(newNode);
        m_trail.append(item);
        explore(item);
    }
}

void DiscoveryPanel::slotShow()
{
    if (!m_trail.isEmpty())
        return;

    QXmppDiscoveryIq::Item item;
    item.setJid(m_client->configuration().domain());
    m_trail.append(item);
    explore(item);
}

// PLUGIN

class DiscoveryPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Service discovery"; };
};

bool DiscoveryPlugin::initialize(Chat *chat)
{
    /* register panel */
    DiscoveryPanel *discovery = new DiscoveryPanel(chat, chat->client(), chat);
    discovery->setObjectName(DISCOVERY_ROSTER_ID);
    chat->addPanel(discovery);

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_B), chat);
    connect(shortcut, SIGNAL(activated()), discovery, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(discovery, DiscoveryPlugin)
