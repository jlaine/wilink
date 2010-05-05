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

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppUtils.h"

#include "chat_history.h"
#include "chat_room.h"
#include "chat_roster.h"
#include "chat_roster_item.h"

enum MembersColumns {
    JidColumn = 0,
    AffiliationColumn,
};

ChatRoom::ChatRoom(QXmppClient *xmppClient, const QString &jid, QWidget *parent)
    : ChatConversation(jid, parent), client(xmppClient), joined(false), notifyMessages(false)
{
    setWindowIcon(QIcon(":/chat.png"));
    connect(client, SIGNAL(connected()), this, SLOT(join()));
    connect(client, SIGNAL(connected()), this, SIGNAL(registerPanel()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(client, SIGNAL(discoveryIqReceived(QXmppDiscoveryIq)), this, SLOT(discoveryIqReceived(QXmppDiscoveryIq)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(this, SIGNAL(showPanel()), this, SLOT(join()));
}

void ChatRoom::disconnected()
{
    joined = false;
}

void ChatRoom::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.from() != chatRemoteJid ||
        disco.type() != QXmppIq::Result ||
        disco.queryType() != QXmppDiscoveryIq::InfoQuery)
        return;

    // notify user of received messages if the room is not publicly listed
    if (disco.features().contains("muc_hidden"))
        notifyMessages = true;
}

/** Send a request to join a multi-user chat.
 */
void ChatRoom::join()
{
    if (joined)
        return;

    // clear history
    chatHistory->clear();

    // request room information
    QXmppDiscoveryIq disco;
    disco.setTo(chatRemoteJid);
    disco.setQueryType(QXmppDiscoveryIq::InfoQuery);
    client->sendPacket(disco);
    
    // send join request
    QXmppPresence packet;
    packet.setTo(chatRemoteJid + "/" + chatLocalName);
    packet.setType(QXmppPresence::Available);
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_muc);
    packet.setExtensions(x);
    client->sendPacket(packet);
    
    joined = true;
}

/** Send a request to leave a multi-user chat.
 */
void ChatRoom::leave()
{
    QXmppPresence packet;
    packet.setTo(chatRemoteJid + "/" + chatLocalName);
    packet.setType(QXmppPresence::Unavailable);
    client->sendPacket(packet);
}

void ChatRoom::messageReceived(const QXmppMessage &msg)
{
    const QString from = jidToResource(msg.from());
    if (jidToBareJid(msg.from()) != chatRemoteJid || from.isEmpty())
        return;

    ChatHistoryMessage message;
    message.body = msg.body();
    message.from = from;
    message.received = (from != chatLocalName);
    message.date = QDateTime::currentDateTime();
    foreach (const QXmppElement &extension, msg.extensions())
    {
        if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_delay)
        {
            const QString str = extension.attribute("stamp");
            message.date = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
            message.date.setTimeSpec(Qt::UTC);
        }
    }
    chatHistory->addMessage(message);

    // notify
    if (notifyMessages)
        emit notifyPanel();    
}

void ChatRoom::presenceReceived(const QXmppPresence &presence)
{
    if (jidToBareJid(presence.from()) != chatRemoteJid)
        return;

    switch (presence.getType())
    {
    case QXmppPresence::Error:
        foreach (const QXmppElement &extension, presence.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc)
            {
                // leave room
                emit hidePanel();

                QXmppStanza::Error error = presence.error();
                QMessageBox::warning(window(),
                    tr("Chat room error"),
                    tr("Sorry, but you cannot join chat room %1.\n\n%2")
                        .arg(chatRemoteJid)
                        .arg(error.text()));
                break;
            }
        }
        break;
    case QXmppPresence::Unavailable:
        if (chatLocalName != jidToResource(presence.from()))
            return;

        // leave room
        emit hidePanel();

        foreach (const QXmppElement &extension, presence.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc_user)
            {
                int statusCode = extension.firstChildElement("status").attribute("code").toInt();
                if (statusCode == 307)
                {
                    QXmppElement reason = extension.firstChildElement("item").firstChildElement("reason");
                    QMessageBox::warning(window(),
                        tr("Chat room error"),
                        tr("Sorry, but you were kicked from chat room %1.\n\n%2")
                            .arg(chatRemoteJid)
                            .arg(reason.value()));
                }
                break;
            }
        }
        break;
    }
}

void ChatRoom::sendMessage(const QString &text)
{
    QXmppMessage msg;
    msg.setBody(text);
    msg.setFrom(chatRemoteJid + "/" + chatLocalName);
    msg.setTo(chatRemoteJid);
    msg.setType(QXmppMessage::GroupChat);
    client->sendPacket(msg);
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
    if (disco.type() == QXmppIq::Result &&
        disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == chatRoomServer)
    {
        // chat rooms list
        listWidget->clear();
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), item.name());
            wdgItem->setData(Qt::UserRole, item.jid());
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

ChatRoomInvitePrompt::ChatRoomInvitePrompt(const QString &contactName, const QString &roomJid, QWidget *parent)
    : QMessageBox(parent), m_jid(roomJid)
{
    setText(tr("%1 has asked to add you to join the '%2' chat room.\n\nDo you accept?").arg(contactName, roomJid));
    setWindowModality(Qt::NonModal);
    setWindowTitle(tr("Invitation from %1").arg(contactName));

    setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    setDefaultButton(QMessageBox::Yes);
    setEscapeButton(QMessageBox::No);

    connect(this, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotButtonClicked(QAbstractButton*)));
}

void ChatRoomInvitePrompt::slotButtonClicked(QAbstractButton *button)
{
    if (standardButton(button) == QMessageBox::Yes)
        emit itemAction(ChatRosterView::JoinAction, m_jid, ChatRosterItem::Room);
}

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
    tableWidget->horizontalHeader()->setResizeMode(JidColumn, QHeaderView::Stretch);

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

    affiliations["member"] = tr("member");
    affiliations["admin"] = tr("administrator");
    affiliations["owner"] = tr("owner");
    affiliations["outcast"] = tr("banned");
    foreach (const QString &affiliation, affiliations.keys())
    {
        QXmppElement item;
        item.setTagName("item");
        item.setAttribute("affiliation", affiliation);

        QXmppElement query;
        query.setTagName("query");
        query.setAttribute("xmlns", ns_muc_admin);
        query.appendChild(item);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setExtensions(query);
        client->sendPacket(iq);
    }
}

void ChatRoomMembers::iqReceived(const QXmppIq &iq)
{
    if (iq.type() != QXmppIq::Result ||
        iq.from() != chatRoomJid)
        return;
    const QXmppElement query = iq.extensions().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_admin)
        return;

    QXmppElement element = query.firstChildElement();
    while (!element.isNull())
    {
        const QString jid = element.attribute("jid");
        const QString affiliation = element.attribute("affiliation");
        if (!initialMembers.contains(jid))
        {
            addEntry(jid, affiliation);
            initialMembers[jid] = affiliation;
        }
        element = element.nextSiblingElement();
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
        foreach (const QXmppElement &child, elements)
            query.appendChild(child);

        QXmppIq iq;
        iq.setTo(chatRoomJid);
        iq.setType(QXmppIq::Set);
        iq.setExtensions(query);
        client->sendPacket(iq);
    }
    accept();
}

void ChatRoomMembers::addMember()
{
    bool ok = false;
    QString jid = "@" + client->getConfiguration().domain();
    jid = QInputDialog::getText(this, tr("Add a user"),
                  tr("Enter the address of the user you want to add."),
                  QLineEdit::Normal, jid, &ok).toLower();
    if (ok)
        addEntry(jid, "member");
}

void ChatRoomMembers::addEntry(const QString &jid, const QString &affiliation)
{
    QComboBox *combo = new QComboBox;
    foreach (const QString &key, affiliations.keys())
        combo->addItem(affiliations[key], key);
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
