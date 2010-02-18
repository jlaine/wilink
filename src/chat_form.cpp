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
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppMessage.h"

#include "chat_form.h"

ChatRoomOptions::ChatRoomOptions(QXmppClient *xmppClient, const QString &roomJid, QWidget *parent)
    : QDialog(parent), chatRoomJid(roomJid), client(xmppClient)
{
    QVBoxLayout *layout = new QVBoxLayout;

    frame = new QFrame;
    layout->addWidget(frame);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    setLayout(layout);

    setWindowTitle(tr("Chat room options"));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));

    // get room info
    QXmppIq iq;
    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_owner);
    iq.setItems(query);
    iq.setTo(chatRoomJid);
    client->sendPacket(iq);
}

void ChatRoomOptions::iqReceived(const QXmppIq &iq)
{
    if (iq.getType() != QXmppIq::Result ||
        iq.getFrom() != chatRoomJid)
        return;
    const QXmppElement query = iq.getItems().first();
    if (query.tagName() != "query" || query.attribute("xmlns") != ns_muc_owner)
        return;
    form = query.firstChild("x");
    if (form.attribute("type") != "form" || form.attribute("xmlns") != "jabber:x:data")
        return;

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(0);
    foreach (const QXmppElement &field, form.children())
    {
        const QString key = field.attribute("var");
        const QString label = field.attribute("label");
        const QString value = field.firstChild("value").value();
        if (field.tagName() == "field" && !key.isEmpty())
        {
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = new QCheckBox(label);
                checkbox->setChecked(value == "1");
                checkbox->setObjectName(key);
                vbox->addWidget(checkbox);
            } else if (field.attribute("type") == "text-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(label));
                QLineEdit *edit = new QLineEdit(value);
                edit->setObjectName(key);
                hbox->addWidget(edit);
                vbox->addItem(hbox);
            } else if (field.attribute("type") == "list-single") {
                QHBoxLayout *hbox = new QHBoxLayout;
                hbox->addWidget(new QLabel(label));
                QComboBox *combo = new QComboBox;
                combo->setObjectName(key);
                int currentIndex = 0;
                int index = 0;
                foreach (const QXmppElement &option, field.children())
                {
                    if (option.tagName() == "option")
                    {
                        const QString optionValue = option.firstChild("value").value();
                        combo->addItem(option.attribute("label"), optionValue);
                        if (optionValue == value)
                            currentIndex = index;
                        index++;
                    }
                }
                combo->setCurrentIndex(currentIndex);
                hbox->addWidget(combo);
                vbox->addItem(hbox);
            }
        }
    }
    frame->setLayout(vbox);
}

void ChatRoomOptions::submit()
{
    form.setAttribute("type", "submit");
    QXmppElementList formFields;
    foreach (QXmppElement field, form.children())
    {
        if (field.tagName() == "field" && !field.attribute("var").isEmpty())
        {
            const QString key = field.attribute("var");
            QXmppElement value = field.firstChild("value");
            if (field.attribute("type") == "boolean")
            {
                QCheckBox *checkbox = frame->findChild<QCheckBox*>(key);
                value.setValue(checkbox->checkState() == Qt::Checked ? "1" : "0");
                field.setChildren(value);
            } else if (field.attribute("type") == "text-single") {
                QLineEdit *edit = frame->findChild<QLineEdit*>(key);
                value.setValue(edit->text());
                field.setChildren(value);
            } else if (field.attribute("type") == "list-single") {
                QXmppElementList childElements;
                foreach (QXmppElement option, field.children())
                {
                    if (option.tagName() == "value")
                    {
                        QComboBox *combo = frame->findChild<QComboBox*>(key);
                        option.setValue(combo->itemData(combo->currentIndex()).toString());
                    }
                    childElements.append(option);
                }
                field.setChildren(childElements);
            }
        }
        formFields.append(field);
    }
    form.setChildren(formFields);

    QXmppElement query;
    query.setTagName("query");
    query.setAttribute("xmlns", ns_muc_owner);
    query.setChildren(form);

    QXmppIq iq(QXmppIq::Set);
    iq.setItems(query);
    iq.setTo(chatRoomJid);
    client->sendPacket(iq);

    accept();
}
