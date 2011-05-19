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
#include <QCoreApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDesktopServices>
#include <QDomDocument>
#include <QLayout>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "QXmppUtils.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "chat.h"
#include "chat_history.h"
#include "chat_plugin.h"

#include "phone/models.h"
#include "phone/sip.h"
#include "phone.h"
 
PhonePanel::PhonePanel(Chat *chatWindow, QWidget *parent)
    : ChatPanel(parent),
    m_window(chatWindow),
    m_registeredHandler(false)
{
    bool check;
    client = chatWindow->client();

    setWindowIcon(QIcon(":/phone.png"));
    setWindowTitle(tr("Phone"));

    // http access
    network = new QNetworkAccessManager(this);

    // sip client
    sip = new SipClient;
    sip->setAudioInputDevice(wApp->audioInputDevice());
    sip->setAudioOutputDevice(wApp->audioOutputDevice());
    check = connect(wApp, SIGNAL(audioInputDeviceChanged(QAudioDeviceInfo)),
                    sip, SLOT(setAudioInputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    check = connect(wApp, SIGNAL(audioOutputDeviceChanged(QAudioDeviceInfo)),
                    sip, SLOT(setAudioOutputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    sip->moveToThread(wApp->soundThread());

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());

    // history
    callsModel = new PhoneCallsModel(sip, network, this);

    // declarative
    declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("historyModel", callsModel);
    context->setContextProperty("sipClient", sip);
    context->setContextProperty("window", m_window);

    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/PhonePanel.qml"));

    layout->addWidget(declarativeView, 1);

    setLayout(layout);
    setFocusProxy(declarativeView);

    check = connect(sip, SIGNAL(callDialled(SipCall*)),
                    callsModel, SLOT(addCall(SipCall*)));
    Q_ASSERT(check);

    check = connect(sip, SIGNAL(callReceived(SipCall*)),
                    this, SLOT(callReceived(SipCall*)));
    Q_ASSERT(check);

    check = connect(sip, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    client, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    // connect signals
    check = connect(client, SIGNAL(connected()),
                    this, SLOT(getSettings()));
    Q_ASSERT(check);

    // add action
    action = m_window->addAction(QIcon(":/phone.png"), tr("Phone"));
    action->setVisible(false);
    check = connect(action, SIGNAL(triggered()),
                    this, SIGNAL(showPanel()));
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
    delete sip;
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
            setWindowHelp(QString("<html>%1 <a href=\"%2\">%3</a></html>").arg(
                                  tr("You can subscribe to the phone service at the following address:"), selfcareUrl, selfcareUrl));
            action->setVisible(true);
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

    // enable action
    action->setVisible(true);
}

/** Open a SIP URI.
 */
void PhonePanel::openUrl(const QUrl &url)
{
    if (url.scheme() != "sip")
        return;

    const QString phoneNumber = url.path().split('@').first();
    const QString recipient = QString("\"%1\" <%2>").arg(phoneNumber, url.toString());
    if (callsModel->call(recipient))
        emit showPanel();
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

    qmlRegisterUncreatableType<SipClient>("wiLink", 1, 2, "SipClient", "");
    qmlRegisterUncreatableType<SipCall>("wiLink", 1, 2, "SipCall", "");

    /* register panel */
    PhonePanel *panel = new PhonePanel(chat);
    chat->addPanel(panel);

    return true;
}

Q_EXPORT_STATIC_PLUGIN2(phone, PhonePlugin)

