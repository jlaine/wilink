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

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>

#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"

#include "chat.h"
#include "chat_edit.h"
#include "chat_history.h"
#include "chat_room.h"

enum MembersColumns {
    JidColumn = 0,
    AffiliationColumn,
};

ChatRoom::ChatRoom(const QString &jid, QWidget *parent)
    : ChatDialog(jid, parent)
{
}

bool ChatRoom::isRoom() const
{
    return true;
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QStringList bits = msg.getFrom().split("/");
    if (bits.size() != 2)
        return;
    const QString from = bits[1];

    ChatHistoryMessage message;
    message.body = msg.getBody();
    message.from = from;
    message.local = (from == chatLocalName);
    if (msg.getExtension().attribute("xmlns") == ns_delay)
    {
        const QString str = msg.getExtension().attribute("stamp");
        message.datetime = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
        message.datetime.setTimeSpec(Qt::UTC);
    } else {
        message.datetime = QDateTime::currentDateTime();
    }
    chatHistory->addMessage(message);
}

void ChatRoom::sendMessage(const QString &text)
{
    QXmppMessage msg;
    msg.setBody(text);
    msg.setFrom(chatRemoteJid + "/" + chatLocalName);
    msg.setTo(chatRemoteJid);
    msg.setType(QXmppMessage::GroupChat);
    emit sendPacket(msg);
}

ChatRoomPrompt::ChatRoomPrompt(QXmppClient *client, const QString &roomServer, QWidget *parent)
    : QDialog(parent), chatRoomServer(roomServer)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Enter the name of the chat room you want to join.")));
    lineEdit = new QLineEdit;
    layout->addWidget(lineEdit);

    listWidget = new QListWidget;
    listWidget->setIconSize(QSize(32, 32));
    listWidget->setSortingEnabled(true);
    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
    layout->addWidget(listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    setLayout(layout);

    setWindowTitle(tr("Join a chat room"));
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));

    // get rooms
    QXmppDiscoveryIq disco;
    disco.setTo(chatRoomServer);
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    client->sendPacket(disco);
}

void ChatRoomPrompt::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.getType() == QXmppIq::Result &&
        disco.getQueryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.getFrom() == chatRoomServer)
    {
        // chat rooms list
        listWidget->clear();
        foreach (const QXmppElement &item, disco.getQueryItems())
        {
            QString jid = item.attribute("jid");
            QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), item.attribute("name"));
            wdgItem->setData(Qt::UserRole, jid);
            listWidget->addItem(wdgItem);
        }
    }
}

void ChatRoomPrompt::itemClicked(QListWidgetItem *item)
{
    lineEdit->setText(item->data(Qt::UserRole).toString());
    validate();
}

QString ChatRoomPrompt::textValue() const
{
    return lineEdit->text();
}

void ChatRoomPrompt::validate()
{
    QString jid = lineEdit->text();
    if (jid.contains(" ") || jid.isEmpty())
    {
        lineEdit->setText(jid.trimmed().replace(" ", "_"));
        return;
    }
    if (!jid.contains("@"))
        lineEdit->setText(jid.toLower() + "@" + chatRoomServer);
    else
        lineEdit->setText(jid.toLower());
    accept();
}

const QStringList ChatRoomMembers::affiliations = QStringList() << "member" << "admin" << "owner" << "outcast";

ChatRoomMembers::ChatRoomMembers(QXmppClient *xmppClient, const QString &roomJid, QWidget *parent)
    : QDialog(parent), chatRoomJid(roomJid), client(xmppClient)
{
    QVBoxLayout *layout = new QVBoxLayout;

    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderItem(JidColumn, new QTableWidgetItem(tr("User")));
    tableWidget->setHorizontalHeaderItem(AffiliationColumn, new QTableWidgetItem(tr("Role")));
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);

    layout->addWidget(tableWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addMember()));
    buttonBox->addButton(addButton, QDialogButtonBox::ActionRole);

    QPushButton *removeButton = new QPushButton;
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);
    setLayout(layout);

    setWindowTitle(tr("Chat room members"));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));

    foreach (const QString &affiliation, ChatRoomMembers::affiliations)
    {
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("affiliation", affiliation);

        QXmppElement query;
        query.setTagName("query");
        query.setAttribute("xmlns", ns_muc_admin);
        query.setChildren(item);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setItems(query);
        client->sendPacket(iq);
    }
}

void ChatRoomMembers::iqReceived(const QXmppIq &iq)
{
    if (iq.getType() != QXmppIq::Result ||
        iq.getFrom() != chatRoomJid)
        return;
    const QXmppElement query = iq.getItems().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_admin)
        return;

    foreach(QXmppElement element, query.children())
    {
        // FIXME: check that initialMembers does not already have the current jid ?
        addEntry(element.attribute("jid"), element.attribute("affiliation"), element.firstChild("reason").value());
        initialMembers[element.attribute("jid")] = element.attribute("affiliation");
    }
    tableWidget->sortItems(JidColumn, Qt::AscendingOrder);;
}

void ChatRoomMembers::submit()
{
    QXmppElementList elements;
    for (int i=0; i < tableWidget->rowCount(); i++)
    {
        const QComboBox *combo = qobject_cast<QComboBox *>(tableWidget->cellWidget(i, AffiliationColumn));
        Q_ASSERT(tableWidget->item(i, JidColumn) && combo);

        const QString currentJid = tableWidget->item(i, JidColumn)->text();
        const QString currentAffiliation = combo->itemData(combo->currentIndex()).toString();
        if (initialMembers.value(currentJid) != currentAffiliation) {
            QXmppElement item;
            item.setTagName("item");
            item.setAttribute("affiliation", currentAffiliation);
            item.setAttribute("jid", currentJid);
            elements.append(item);
        }
        initialMembers.remove(currentJid);
    }

    // Process deleted members (i.e. remaining entries in initialMember)
    foreach(const QString &entry, initialMembers.keys())
    {
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("affiliation", "none");
        item.setAttribute("jid", entry);
        elements.append(item);
    }

    if (! elements.isEmpty()) {
        QXmppElement query;
        query.setTagName("query");
        query.setAttribute("xmlns", ns_muc_admin);
        query.setChildren(elements);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setType(QXmppIq::Set);
        iq.setItems(query);
        client->sendPacket(iq);
    }
    accept();
}

void ChatRoomMembers::addMember()
{
    bool ok = false;
    QString jid = "@" + client->getConfiguration().getDomain();
    jid = QInputDialog::getText(this, tr("Add a user"),
                  tr("Enter the address of the user you want to add."),
                  QLineEdit::Normal, jid, &ok).toLower();
    if (ok)
        addEntry(jid, "member");
}

void ChatRoomMembers::addEntry(const QString &jid, const QString &affiliation, const QString &comment)
{
    QComboBox *combo = new QComboBox;
    foreach (const QString &text, affiliations)
        combo->addItem(text, text); // FIXME: manage translations
    combo->setEditable(false);
    combo->setCurrentIndex(combo->findData(affiliation));
    QTableWidgetItem *jidItem = new QTableWidgetItem(jid);
    jidItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    tableWidget->insertRow(0);
    tableWidget->setCellWidget(0, AffiliationColumn, combo);
    tableWidget->setItem(0, JidColumn, jidItem);
}

void ChatRoomMembers::removeMember()
{
    tableWidget->removeRow(tableWidget->currentRow());
}
