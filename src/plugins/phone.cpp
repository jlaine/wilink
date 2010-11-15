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

#include <QGraphicsScene>
#include <QGraphicsView>
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

    QSettings settings;
    QHBoxLayout *passwordBox = new QHBoxLayout;
    passwordBox->addWidget(new QLabel(tr("Password")));
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setText(settings.value("PhonePassword").toString());
    passwordBox->addWidget(passwordEdit);
    QPushButton *passwordButton = new QPushButton(tr("OK"));
    passwordBox->addWidget(passwordButton);
    layout->addLayout(passwordBox);

    QHBoxLayout *hbox = new QHBoxLayout;
    numberEdit = new QLineEdit;
    numberEdit->setText(settings.value("PhoneHistory").toString());
    hbox->addWidget(numberEdit);
    setFocusProxy(numberEdit);
    QPushButton *backspaceButton = new QPushButton;
    backspaceButton->setIcon(QIcon(":/back.png"));
    hbox->addWidget(backspaceButton);
    callButton = new QPushButton(tr("Call"));
    callButton->setIcon(QIcon(":/call.png"));
    callButton->setEnabled(false);
    hbox->addWidget(callButton);
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
        connect(key, SIGNAL(clicked()), this, SLOT(keyPressed()));
        grid->addWidget(key, i / 3, i % 3, 1, 1);
    }
    layout->addLayout(grid);

    // view
    QGraphicsView *graphicsView = new QGraphicsView;
    graphicsView->setScene(new QGraphicsScene(graphicsView));
    layout->addWidget(graphicsView);

    callBar = new ChatPanelBar(graphicsView);
    callBar->setZValue(10);
    graphicsView->scene()->addItem(callBar);

    // status
    statusLabel = new QLabel;
    layout->addWidget(statusLabel);

    setLayout(layout);

    sip = new SipClient(this);

    /* connect signals */
    connect(backspaceButton, SIGNAL(clicked()), this, SLOT(backspacePressed()));
    connect(client, SIGNAL(connected()), this, SLOT(chatConnected()));
    connect(sip, SIGNAL(connected()), this, SLOT(sipConnected()));
    connect(sip, SIGNAL(disconnected()), this, SLOT(sipDisconnected()));
    connect(sip, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
            client, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    connect(numberEdit, SIGNAL(returnPressed()), this, SLOT(callNumber()));
    connect(callButton, SIGNAL(clicked()), this, SLOT(callNumber()));
    connect(passwordEdit, SIGNAL(returnPressed()), this, SLOT(passwordPressed()));
    connect(passwordButton, SIGNAL(clicked()), this, SLOT(passwordPressed()));
}

void PhonePanel::addWidget(ChatPanelWidget *widget)
{
    callBar->addWidget(widget);
}

void PhonePanel::backspacePressed()
{
    numberEdit->backspace();
}

void PhonePanel::callNumber()
{
    QString phoneNumber = numberEdit->text().trimmed();
    if (!callButton->isEnabled() || phoneNumber.isEmpty())
        return;

    const QString recipient = QString("sip:%1@%2").arg(phoneNumber, sip->serverName());

    SipCall *call = sip->call(recipient);
    if (!call)
        return;

    addWidget(new PhoneWidget(call));

    // remember number
    QSettings settings;
    settings.setValue("PhoneHistory", phoneNumber);
}

void PhonePanel::chatConnected()
{
    const QString jid = client->configuration().jid();
    sip->setDomain(jidToDomain(jid));
    sip->setUsername(jidToUser(jid));

    statusLabel->setText(tr("Connecting.."));
    sip->setPassword(passwordEdit->text());
    sip->connectToServer();
    emit registerPanel();
}

void PhonePanel::keyPressed()
{
    QPushButton *key = qobject_cast<QPushButton*>(sender());
    if (!key)
        return;
    numberEdit->insert(key->text());
}

void PhonePanel::passwordPressed()
{
    const QString password = passwordEdit->text();

    QSettings settings;
    settings.setValue("PhonePassword", password);
    statusLabel->setText(tr("Connecting.."));
    sip->setPassword(password);
    sip->connectToServer();
}

void PhonePanel::sipConnected()
{
    statusLabel->setText(tr("Connected."));
    callButton->setEnabled(true);
}

void PhonePanel::sipDisconnected()
{
    statusLabel->setText(tr("Disconnected."));
    callButton->setEnabled(false);
}

// WIDGET

PhoneWidget::PhoneWidget(SipCall *call, QGraphicsItem *parent)
    : ChatPanelWidget(parent),
    m_call(call)
{
    // setup GUI
    setIconPixmap(QPixmap(":/call.png"));
    setButtonPixmap(QPixmap(":/hangup.png"));
    setButtonToolTip(tr("Hang up"));

    QString recipient = call->recipient();
    QRegExp recipientRx("[^>]*<(sip:)?([^>]+)>");
    if (recipientRx.exactMatch(recipient))
        recipient = recipientRx.cap(2).split('@').first();
    m_nameLabel = new QGraphicsSimpleTextItem(recipient, this);
    m_statusLabel = new QGraphicsSimpleTextItem(tr("Connecting.."), this);

    connect(this, SIGNAL(buttonClicked()), m_call, SLOT(hangup()));
    connect(m_call, SIGNAL(finished()), this, SLOT(callFinished()));
    connect(m_call, SIGNAL(ringing()), this, SLOT(callRinging()));
    connect(m_call, SIGNAL(stateChanged(QXmppCall::State)),
        this, SLOT(callStateChanged(QXmppCall::State)));
}

/** When the call thread finishes, perform cleanup.
 */
void PhoneWidget::callFinished()
{
    m_call->deleteLater();
    m_call = 0;

    // make widget disappear
    QTimer::singleShot(1000, this, SLOT(disappear()));
}

void PhoneWidget::callRinging()
{
    m_statusLabel->setText(tr("Ringing.."));
}

void PhoneWidget::callStateChanged(QXmppCall::State state)
{
    // update status
    switch (state)
    {
    case QXmppCall::OfferState:
    case QXmppCall::ConnectingState:
        m_statusLabel->setText(tr("Connecting.."));
        setButtonEnabled(false);
        break;
    case QXmppCall::ActiveState:
        m_statusLabel->setText(tr("Call connected."));
        setButtonEnabled(true);
        break;
    case QXmppCall::DisconnectingState:
        m_statusLabel->setText(tr("Disconnecting.."));
        setButtonEnabled(false);
        break;
    case QXmppCall::FinishedState:
        m_statusLabel->setText(tr("Call finished."));
        setButtonEnabled(false);
        break;
    }
}

void PhoneWidget::setGeometry(const QRectF &rect)
{
    ChatPanelWidget::setGeometry(rect);
    QRectF contents = contentsRect();

    m_nameLabel->setPos(contents.topLeft());
    contents.setTop(m_nameLabel->boundingRect().bottom());
 
    m_statusLabel->setPos(contents.left(), contents.top() +
        (contents.height() - m_statusLabel->boundingRect().height()) / 2);
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

