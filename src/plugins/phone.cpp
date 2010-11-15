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

#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "phone/sip.h"
#include "phone.h"
 
#define PHONE_ROSTER_ID "0_phone"

PhonePanel::PhonePanel(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent),
    client(xmppClient)
{
    setWindowIcon(QIcon(":/call.png"));
    setWindowTitle(tr("Phone"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(headerLayout());
    //layout->addSpacing(10);

    QHBoxLayout *hbox = new QHBoxLayout;
    numberEdit = new QLineEdit;
    hbox->addWidget(numberEdit);
    callButton = new QPushButton(tr("Call"));
    callButton->setIcon(QIcon(":/call.png"));
    hbox->addWidget(callButton);
    hangupButton = new QPushButton(tr("Hang up"));;
    hangupButton->setIcon(QIcon(":/hangup.png"));
    hangupButton->hide();
    hbox->addWidget(hangupButton);
    layout->addLayout(hbox);

    // keyboard
    QGridLayout *grid = new QGridLayout;
    QStringList keys;
    keys << "1" << "2" << "3";
    keys << "4" << "5" << "6";
    keys << "7" << "8" << "9";
    keys << "*" << "0" << "#";
    for (int i = 0; i < keys.size(); ++i) {
        QPushButton *key = new QPushButton(keys[i]);
        grid->addWidget(key, i / 3, i % 3, 1, 1);
    }
    layout->addLayout(grid);

    layout->addStretch();

    statusLabel = new QLabel;
    layout->addWidget(statusLabel);

    setLayout(layout);

    sip = new SipClient(this);

    /* connect signals */
    connect(client, SIGNAL(connected()), this, SLOT(chatConnected()));
    connect(sip, SIGNAL(connected()), this, SLOT(sipConnected()));
    connect(sip, SIGNAL(disconnected()), this, SLOT(sipDisconnected()));
    connect(sip, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
            client, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    connect(numberEdit, SIGNAL(returnPressed()), this, SLOT(callNumber()));
    connect(callButton, SIGNAL(clicked()), this, SLOT(callNumber()));

    /* register panel */
    QTimer::singleShot(0, this, SIGNAL(registerPanel()));
}

void PhonePanel::callConnected()
{
    statusLabel->setText(tr("Call active."));
}

void PhonePanel::callFinished()
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    if (!call)
        return;

    statusLabel->setText(tr("Call finished."));
    hangupButton->hide();
    callButton->show();
    call->deleteLater();
}

void PhonePanel::callNumber()
{
    QString phoneNumber = numberEdit->text().trimmed();
    if (phoneNumber.isEmpty())
        return;

    callButton->hide();
    hangupButton->show();

    const QString recipient = QString("sip:%1@%2").arg(phoneNumber, sip->serverName());
    statusLabel->setText(tr("Calling %1..").arg(phoneNumber));
    SipCall *call = sip->call(recipient);
    connect(call, SIGNAL(connected()),
            this, SLOT(callConnected()));
    connect(call, SIGNAL(finished()),
            this, SLOT(callFinished()));
    connect(call, SIGNAL(ringing()),
            this, SLOT(callRinging()));
    connect(hangupButton, SIGNAL(clicked()), call, SLOT(hangup()));
}

void PhonePanel::callRinging()
{
    statusLabel->setText(tr("Call ringing.."));
}

void PhonePanel::chatConnected()
{
    const QString jid = client->configuration().jid();
    sip->setDomain(jidToDomain(jid));
    sip->setUsername(jidToUser(jid));

    // FIXME : get password
    QSettings settings("sip.conf", QSettings::IniFormat);
    sip->setPassword(settings.value("password").toString());
    numberEdit->setText(settings.value("phoneNumber").toString());

    statusLabel->setText(tr("Connecting.."));
    sip->connectToServer();
}

void PhonePanel::sipConnected()
{
    statusLabel->setText(tr("Connected"));
}

void PhonePanel::sipDisconnected()
{
    statusLabel->setText(tr("Disconnected"));
}

// PLUGIN

class PhonePlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool PhonePlugin::initialize(Chat *chat)
{
    const QString domain = chat->client()->configuration().domain();
    if (domain != "wifirst.net")
        return false;

    /* register panel */
    PhonePanel *panel = new PhonePanel(chat->client());
    panel->setObjectName(PHONE_ROSTER_ID);
    chat->addPanel(panel);

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(phone, PhonePlugin)

