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

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDomDocument>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include "QSoundMeter.h"

#include "QXmppRtpChannel.h"
#include "QXmppUtils.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "chat.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "phone/models.h"
#include "phone/sip.h"
#include "phone.h"
 
#define PHONE_ROSTER_ID "0_phone"

// Builds a full SIP address from a short recipient
static QString buildAddress(const QString &recipient, const QString &sipDomain)
{
    QString address, name;
    if (recipient.contains("@")) {
        name = recipient.split("@").first();
        address = recipient;
    } else {
        name = recipient;
        address = recipient + "@" + sipDomain;
    }
    return QString("\"%1\" <sip:%2>").arg(name, address);
}

// Extracts the shortest possible recipient from a full SIP address.
static QString parseAddress(const QString &sipAddress, const QString &sipDomain)
{
    QRegExp rx(sipAddressPattern);
    if (!rx.exactMatch(sipAddress))
        return QString();
    const QString recipient = rx.cap(2).mid(4);

    QStringList bits = recipient.split("@");
    if (bits.last() == sipDomain || QRegExp("[0-9]+").exactMatch(bits.first()))
        return bits.first();
    else
        return recipient;
}

PhonePanel::PhonePanel(Chat *chatWindow, QWidget *parent)
    : ChatPanel(parent),
    m_window(chatWindow),
    m_registeredHandler(false)
{
    bool check;
    client = chatWindow->client();

    setWindowIcon(QIcon(":/call.png"));
    setWindowTitle(tr("Phone"));

    // http access
    network = new QNetworkAccessManager(this);
    check = connect(network, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                    this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
    Q_ASSERT(check);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(headerLayout());

    // selfcare message
    QVBoxLayout *vbox = new QVBoxLayout;
    selfcareMessage = new QLabel;
    selfcareMessage->setFrameStyle(QFrame::Box);
    selfcareMessage->setOpenExternalLinks(true);
    selfcareMessage->setWordWrap(true);
    selfcareMessage->hide();
    vbox->addWidget(selfcareMessage);
    layout->addLayout(vbox);

    // calls buttons
    QHBoxLayout *hbox = new QHBoxLayout;
    numberEdit = new QLineEdit;
    hbox->addWidget(numberEdit);
    setFocusProxy(numberEdit);
    QPushButton *backspaceButton = new QPushButton;
    backspaceButton->setIcon(QIcon(":/back.png"));
    hbox->addWidget(backspaceButton);
    callButton = new QPushButton(tr("Call"));
    callButton->setIcon(QIcon(":/call.png"));
    callButton->setEnabled(false);
    hbox->addWidget(callButton);
    hangupButton = new QPushButton(tr("Hangup"));
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
        connect(key, SIGNAL(pressed()), this, SLOT(keyPressed()));
        connect(key, SIGNAL(released()), this, SLOT(keyReleased()));
        grid->addWidget(key, i / 3, i % 3, 1, 1);
    }
    QGroupBox *groupBox = new QGroupBox;
    groupBox->setLayout(grid);

    hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(groupBox);

    // input volume bar
    QGridLayout *barBox = new QGridLayout;
    QSoundMeterBar *inputBar = new QSoundMeterBar;
    inputBar->setOrientation(Qt::Vertical);
    barBox->addWidget(inputBar, 0, 0);
    QLabel *inputLabel = new QLabel;
    inputLabel->setPixmap(QPixmap(":/audio-input.png"));
    barBox->addWidget(inputLabel, 1, 0);

    // output volume bar
    QSoundMeterBar *outputBar = new QSoundMeterBar;
    outputBar->setOrientation(Qt::Vertical);
    barBox->addWidget(outputBar, 0, 1);
    QLabel *outputLabel = new QLabel;
    outputLabel->setPixmap(QPixmap(":/audio-output.png"));
    barBox->addWidget(outputLabel, 1, 1);
    hbox->addLayout(barBox);

    hbox->addStretch();
    layout->addLayout(hbox);

    // history
    callsModel = new PhoneCallsModel(network, this);
    check = connect(callsModel, SIGNAL(error(QString)),
                    this, SLOT(error(QString)));
    Q_ASSERT(check);
    check = connect(callsModel, SIGNAL(stateChanged(bool)),
                    this, SLOT(callStateChanged(bool)));
    Q_ASSERT(check);
    check = connect(callsModel, SIGNAL(inputVolumeChanged(int)),
                    inputBar, SLOT(setValue(int)));
    Q_ASSERT(check);
    check = connect(callsModel, SIGNAL(outputVolumeChanged(int)),
                    outputBar, SLOT(setValue(int)));
    Q_ASSERT(check);
    callsView = new PhoneCallsView(callsModel, this);
    check = connect(callsView, SIGNAL(clicked(QModelIndex)),
                    this, SLOT(historyClicked(QModelIndex)));
    Q_ASSERT(check);
    check = connect(callsView, SIGNAL(doubleClicked(QModelIndex)),
                    this, SLOT(historyDoubleClicked(QModelIndex)));
    Q_ASSERT(check);
    layout->addWidget(callsView, 1);

    // status
    statusLabel = new QLabel;
    layout->addWidget(statusLabel);

    setLayout(layout);

    // sip client
    sipThread = new QThread(this);
    sip = new SipClient;
    sip->setAudioInputDevice(wApp->audioInputDevice());
    sip->setAudioOutputDevice(wApp->audioOutputDevice());
    sip->moveToThread(sipThread);
    check = connect(wApp, SIGNAL(audioInputDeviceChanged(QAudioDeviceInfo)),
                    sip, SLOT(setAudioInputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    check = connect(wApp, SIGNAL(audioOutputDeviceChanged(QAudioDeviceInfo)),
                    sip, SLOT(setAudioOutputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    sipThread->start();

    check = connect(sip, SIGNAL(callDialled(SipCall*)),
                    callsModel, SLOT(addCall(SipCall*)));
    Q_ASSERT(check);

    check = connect(sip, SIGNAL(callReceived(SipCall*)),
                    this, SLOT(callReceived(SipCall*)));
    Q_ASSERT(check);

    check = connect(sip, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    client, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    check = connect(sip, SIGNAL(stateChanged(SipClient::State)),
                    this, SLOT(sipStateChanged(SipClient::State)));
    Q_ASSERT(check);

    // connect signals
    check = connect(backspaceButton, SIGNAL(clicked()),
                    this, SLOT(backspacePressed()));
    Q_ASSERT(check);
    check = connect(client, SIGNAL(connected()),
                    this, SLOT(getSettings()));
    Q_ASSERT(check);
    check = connect(numberEdit, SIGNAL(returnPressed()),
                    this, SLOT(callNumber()));
    Q_ASSERT(check);
    check = connect(callButton, SIGNAL(clicked()),
                    this, SLOT(callNumber()));
    Q_ASSERT(check);
    check = connect(hangupButton, SIGNAL(clicked()),
                    callsModel, SLOT(hangup()));
    Q_ASSERT(check);
}

PhonePanel::~PhonePanel()
{
    // give SIP client 5s to exit cleanly
    if (sip->state() == SipClient::ConnectedState) {
        QEventLoop loop;
        QTimer::singleShot(5000, &loop, SLOT(quit()));
        connect(sip, SIGNAL(disconnected()), &loop, SLOT(quit()));
        QMetaObject::invokeMethod(sip, "disconnectFromServer");
        loop.exec();
    }
    sipThread->quit();
    sipThread->wait();
    delete sip;
}

void PhonePanel::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    QNetIO::Wallet::instance()->onAuthenticationRequired("www.wifirst.net", authenticator);
}

void PhonePanel::backspacePressed()
{
    numberEdit->backspace();
}

void PhonePanel::callButtonClicked(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    SipCall *call = qobject_cast<SipCall*>(box->property("call").value<QObject*>());
    if (!call)
        return;

    if (box->standardButton(button) == QMessageBox::Yes)
        QMetaObject::invokeMethod(call, "accept");
    else
        QMetaObject::invokeMethod(call, "hangup");
    box->deleteLater();
}

void PhonePanel::callNumber()
{
    const QString recipient = numberEdit->text().replace(QRegExp("\\s+"), QString());
    if (sip->state() != SipClient::ConnectedState ||
        !callsModel->activeCalls().isEmpty() ||
        recipient.isEmpty())
        return;

    numberEdit->clear();
    const QString address = buildAddress(recipient, sip->domain());
    QMetaObject::invokeMethod(sip, "call", Q_ARG(QString, address));
}

void PhonePanel::callReceived(SipCall *call)
{
    callsModel->addCall(call);
    const QString contactName = sipAddressToName(call->recipient());

    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Call from %1").arg(contactName),
        tr("%1 wants to talk to you.\n\nDo you accept?").arg(contactName),
        QMessageBox::Yes | QMessageBox::No, m_window);
    box->setDefaultButton(QMessageBox::NoButton);
    box->setProperty("call", qVariantFromValue(qobject_cast<QObject*>(call)));

    /* connect signals */
    connect(call, SIGNAL(destroyed(QObject*)), box, SLOT(deleteLater()));
    connect(box, SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(callButtonClicked(QAbstractButton*)));
    box->show();
}

void PhonePanel::callStateChanged(bool haveCalls)
{
    if (haveCalls) {
        callButton->hide();
        hangupButton->show();
    } else {
        hangupButton->hide();
        callButton->show();
    }
}

void PhonePanel::error(const QString &error)
{
    QMessageBox::warning(this, tr("Call failed"), tr("Sorry, but the call could not be completed.") + "\n\n" + error);
}

/** Requests VoIP settings from the server.
 */
void PhonePanel::getSettings()
{
    QNetworkRequest req(QUrl("https://www.wifirst.net/wilink/voip"));
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(handleSettings()));
}

/** Handles VoIP settings received from the server.
 */
void PhonePanel::handleSettings()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Failed to retrieve phone settings: %s", qPrintable(reply->errorString()));
        return;
    }

    QDomDocument doc;
    doc.setContent(reply);
    QDomElement settings = doc.documentElement();

    // parse settings from server
    const bool enabled = settings.firstChildElement("enabled").text() == "true";
    const QString domain = settings.firstChildElement("domain").text();
    const QString username = settings.firstChildElement("username").text();
    const QString password = settings.firstChildElement("password").text();
    const QString number = settings.firstChildElement("number").text();
    const QString callsUrl = settings.firstChildElement("calls-url").text();
    const QString selfcareUrl = settings.firstChildElement("selfcare-url").text();

    // check service is activated
    if (!enabled || domain.isEmpty() || username.isEmpty() || password.isEmpty()) {
        if (!selfcareUrl.isEmpty()) {
            // show a message
            selfcareMessage->setText(QString("<html>%1<br/><a href=\"%2\">%3</a></html>").arg(
                                         tr("You can subscribe to the phone service at the following address:"), selfcareUrl, selfcareUrl));
            selfcareMessage->show();
            emit registerPanel();
        }
        return;
    }

    if (!number.isEmpty())
        setWindowExtra(tr("Your number is %1").arg(number));

    // connect to server
    if (sip->displayName() != number ||
        sip->domain() != domain ||
        sip->username() != username ||
        sip->password() != password)
    {
        sip->setDisplayName(number);
        sip->setDomain(domain);
        sip->setUsername(username);
        sip->setPassword(password);
        QMetaObject::invokeMethod(sip, "connectToServer");
    }

    // register URL handler
    if (!m_registeredHandler) {
        ChatMessage::addTransform(QRegExp("^(.*\\s)?(\\+?[0-9]{4,})(\\s.*)?$"),
            QString("\\1<a href=\"sip:\\2@%1\">\\2</a>\\3").arg(domain));
        QDesktopServices::setUrlHandler("sip", this, "openUrl");
    }

    // retrieve call history
    if (!callsUrl.isEmpty())
        callsModel->setUrl(QUrl(callsUrl));

    emit registerPanel();
}

void PhonePanel::historyClicked(const QModelIndex &index)
{
    const QString recipient = parseAddress(index.data(PhoneCallsModel::AddressRole).toString(), sip->domain());
    if (recipient.isEmpty())
        return;
    numberEdit->setText(recipient);
}

void PhonePanel::historyDoubleClicked(const QModelIndex &index)
{
    const QString recipient = parseAddress(index.data(PhoneCallsModel::AddressRole).toString(), sip->domain());
    if (sip->state() != SipClient::ConnectedState ||
        !callsModel->activeCalls().isEmpty() ||
        recipient.isEmpty())
        return;

    const QString address = buildAddress(recipient, sip->domain());
    QMetaObject::invokeMethod(sip, "call", Q_ARG(QString, address));
}

static QXmppRtpAudioChannel::Tone keyTone(QPushButton *key)
{
    char c = key->text()[0].toLatin1();
    if (c >= '0' && c <= '9')
        return QXmppRtpAudioChannel::Tone(c - '0');
    else if (c == '*')
        return QXmppRtpAudioChannel::Tone_Star;
    else if (c == '#')
        return QXmppRtpAudioChannel::Tone_Pound;
    else
        return QXmppRtpAudioChannel::Tone(-1);
}

void PhonePanel::keyPressed()
{
    QPushButton *key = qobject_cast<QPushButton*>(sender());
    if (!key)
        return;

    QList<SipCall*> calls = callsModel->activeCalls();
    QXmppRtpAudioChannel::Tone tone = keyTone(key);
    foreach (SipCall *call, calls)
        call->audioChannel()->startTone(tone);
}

void PhonePanel::keyReleased()
{
    QPushButton *key = qobject_cast<QPushButton*>(sender());
    if (!key)
        return;
    QList<SipCall*> calls = callsModel->activeCalls();

    // add digit
    if (calls.isEmpty()) {
        numberEdit->insert(key->text());
        return;
    }

    // send DTMF
    QXmppRtpAudioChannel::Tone tone = keyTone(key);
    foreach (SipCall *call, calls)
        call->audioChannel()->stopTone(tone);
}

/** Open a SIP URI.
 */
void PhonePanel::openUrl(const QUrl &url)
{
    if (sip->state() != SipClient::ConnectedState ||
        !callsModel->activeCalls().isEmpty() ||
        url.scheme() != "sip")
        return;

    const QString phoneNumber = url.path().split('@').first();
    const QString recipient = QString("\"%1\" <%2>").arg(phoneNumber, url.toString());
    QMetaObject::invokeMethod(sip, "call", Q_ARG(QString, recipient));
    emit showPanel();
}

void PhonePanel::sipStateChanged(SipClient::State state)
{
    switch (state)
    {
    case SipClient::ConnectingState:
        statusLabel->setText(tr("Connecting.."));
        break;
    case SipClient::ConnectedState:
        statusLabel->setText(tr("Connected."));
        callButton->setEnabled(true);
        break;
    case SipClient::DisconnectingState:
        statusLabel->setText(tr("Disconnecting.."));
        break;
    case SipClient::DisconnectedState:
        statusLabel->setText(tr("Disconnected."));
        callButton->setEnabled(false);
        break;
    }
}

// PLUGIN

class PhonePlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    QString name() const { return "Telephone calls"; };
};

bool PhonePlugin::initialize(Chat *chat)
{
    const QString domain = chat->client()->configuration().domain();
    if (domain != "wifirst.net")
        return false;

    /* register panel */
    PhonePanel *panel = new PhonePanel(chat);
    panel->setObjectName(PHONE_ROSTER_ID);
    chat->addPanel(panel);

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(phone, PhonePlugin)

