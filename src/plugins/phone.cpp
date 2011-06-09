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

#include <QDateTime>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDomElement>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QXmppUtils.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "declarative.h"
#include "history.h"
#include "phone.h"
#include "phone/sip.h"

#define FLAGS_DIRECTION 0x1
#define FLAGS_ERROR 0x2

static QString formatDuration(int secs)
{
    return QString::number(secs) + "s";
}

class PhoneCallsItem
{
public:
    PhoneCallsItem();
    QByteArray data() const;
    void parse(const QDomElement &callElement);

    int id;
    QString address;
    QDateTime date;
    int duration;
    int flags;

    SipCall *call;
    QNetworkReply *reply;
    int soundId;
};

PhoneCallsItem::PhoneCallsItem()
    : id(0),
    duration(0),
    flags(0),
    call(0),
    reply(0),
    soundId(0)
{
}

QByteArray PhoneCallsItem::data() const
{
    QUrl data;
    data.addQueryItem("address", address);
    data.addQueryItem("duration", QString::number(duration));
    data.addQueryItem("flags", QString::number(flags));
    return data.encodedQuery();
}

void PhoneCallsItem::parse(const QDomElement &callElement)
{
    id = callElement.firstChildElement("id").text().toInt();
    address = callElement.firstChildElement("address").text();
    date = datetimeFromString(callElement.firstChildElement("date").text());
    duration = callElement.firstChildElement("duration").text().toInt();
    flags = callElement.firstChildElement("flags").text().toInt();
}

/** Constructs a new model representing the call history.
 *
 * @param network
 * @param parent
 */
PhoneCallsModel::PhoneCallsModel(QObject *parent)
    : QAbstractListModel(parent),
    m_enabled(false),
    m_registeredHandler(false)
{
    bool check;

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ActiveRole, "active");
    roleNames.insert(AddressRole, "address");
    roleNames.insert(DateRole, "date");
    roleNames.insert(DirectionRole, "direction");
    roleNames.insert(DurationRole, "duration");
    roleNames.insert(IdRole, "id");
    roleNames.insert(NameRole, "name");
    roleNames.insert(StateRole, "state");
    setRoleNames(roleNames);

    // http
    m_network = new NetworkAccessManager(this);

    // sip
    m_client = new SipClient;
    m_client->setAudioInputDevice(wApp->audioInputDevice());
    m_client->setAudioOutputDevice(wApp->audioOutputDevice());
    check = connect(wApp, SIGNAL(audioInputDeviceChanged(QAudioDeviceInfo)),
                    m_client, SLOT(setAudioInputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    check = connect(wApp, SIGNAL(audioOutputDeviceChanged(QAudioDeviceInfo)),
                    m_client, SLOT(setAudioOutputDevice(QAudioDeviceInfo)));
    Q_ASSERT(check);
    check = connect(m_client, SIGNAL(callDialled(SipCall*)),
                    this, SLOT(addCall(SipCall*)));
    Q_ASSERT(check);
    m_client->moveToThread(wApp->soundThread());

    // ticker for call durations
    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, SIGNAL(timeout()),
            this, SLOT(callTick()));

    // periodic settings retrieval
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(_q_getSettings()));
    m_timer->start(0);
}

PhoneCallsModel::~PhoneCallsModel()
{
    m_ticker->stop();
    m_timer->stop();

    // try to exit SIP client cleanly
    if (m_client->state() == SipClient::ConnectedState)
        QMetaObject::invokeMethod(m_client, "disconnectFromServer");
    m_client->deleteLater();

    // delete items
    foreach (PhoneCallsItem *item, m_items)
        delete item;
}

QList<SipCall*> PhoneCallsModel::activeCalls() const
{
    QList<SipCall*> calls;
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            calls << m_items[i]->call;
    return calls;
}

void PhoneCallsModel::addCall(SipCall *call)
{
    PhoneCallsItem *item = new PhoneCallsItem;
    item->address = call->recipient();
    item->flags = call->direction();
    item->call = call;
    connect(item->call, SIGNAL(stateChanged(QXmppCall::State)),
            this, SLOT(callStateChanged(QXmppCall::State)));
    if (item->call->direction() == QXmppCall::IncomingDirection)
        item->soundId = wApp->soundPlayer()->play(":/call-incoming.ogg", true);
    else
        connect(item->call, SIGNAL(ringing()), this, SLOT(callRinging()));
    QNetworkRequest request = buildRequest(m_url);
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    item->reply = m_network->post(request, item->data());
    connect(item->reply, SIGNAL(finished()),
            this, SLOT(_q_handleCreate()));

    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();

    // schedule periodic refresh
    if (!m_ticker->isActive())
        m_ticker->start();

    // notify change
    emit currentCallsChanged();
}

QNetworkRequest PhoneCallsModel::buildRequest(const QUrl &url) const
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/xml");
    return req;
}

bool PhoneCallsModel::call(const QString &address)
{
    if (m_client->state() == SipClient::ConnectedState &&
        !currentCalls()) {
        QMetaObject::invokeMethod(m_client, "call", Q_ARG(QString, address));
        return true;
    }
    return false;
}

void PhoneCallsModel::callRinging()
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the call
    foreach (PhoneCallsItem *item, m_items) {
        if (item->call == call && !item->soundId) {
            item->soundId = wApp->soundPlayer()->play(":/call-outgoing.ogg", true);
            break;
        }
    }
}

void PhoneCallsModel::callStateChanged(QXmppCall::State state)
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the call
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->call == call) {
            row = i;
            break;
        }
    }
    if (row < 0)
        return;

    PhoneCallsItem *item = m_items[row];

    if (item->soundId && state != QXmppCall::ConnectingState) {
        wApp->soundPlayer()->stop(item->soundId);
        item->soundId = 0;
    }

    // update the item
    if (state == QXmppCall::ActiveState) {
        connect(call, SIGNAL(inputVolumeChanged(int)),
                this, SIGNAL(inputVolumeChanged(int)));
        connect(call, SIGNAL(outputVolumeChanged(int)),
                this, SIGNAL(outputVolumeChanged(int)));
    }
    else if (state == QXmppCall::FinishedState) {
        emit inputVolumeChanged(0);
        emit outputVolumeChanged(0);

        item->call = 0;
        item->duration = call->duration();
        if (!call->error().isEmpty()) {
            item->flags |= FLAGS_ERROR;
            emit error(call->error());
        }
        emit currentCallsChanged();

        QUrl url = m_url;
        url.setPath(url.path() + QString::number(item->id) + "/");
        QNetworkRequest request = buildRequest(url);
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        m_network->put(request, item->data());

        call->deleteLater();
    }
    emit dataChanged(createIndex(row, 0), createIndex(row, 0));
}

void PhoneCallsModel::callTick()
{
    bool active = false;
    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items[row]->call) {
            active = true;
            emit dataChanged(createIndex(row, 0), createIndex(row, 0));
        }
    }
    if (!active)
        m_ticker->stop();
}

/** Returns the underlying SIP client.
 *
 * \note Use with care as the SIP client lives in a different thread!
 */
SipClient *PhoneCallsModel::client() const
{
    return m_client;
}

/** Returns the number of columns under the given \a parent.
 *
 * @param parent
 */
int PhoneCallsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

int PhoneCallsModel::currentCalls() const
{
    return activeCalls().size();
}

QVariant PhoneCallsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row > m_items.size())
        return QVariant();
    PhoneCallsItem *item = m_items[row];

    if (role == Qt::ToolTipRole) {
        return item->address;
    } else if (role == AddressRole) {
        return item->address;
    } else if (role == ActiveRole) {
        return item->call != 0;
    } else if (role == DateRole) {
        return item->date;
    } else if (role == DirectionRole) {
        return item->flags & FLAGS_DIRECTION;
    } else if (role == DurationRole) {
        return item->call ? item->call->duration() : item->duration;
    } else if (role == IdRole) {
        return item->id;
    } else if (role == NameRole) {
        return sipAddressToName(item->address);
    }

    return QVariant();
}

/** Returns true if the service is active.
 */
bool PhoneCallsModel::enabled() const
{
    return m_enabled;
}

/** Hangs up all active calls.
 */
void PhoneCallsModel::hangup()
{
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            QMetaObject::invokeMethod(m_items[i]->call, "hangup");
}

int PhoneCallsModel::inputVolume() const
{
    QList<SipCall*> calls = activeCalls();
    return calls.isEmpty() ? 0 : calls.first()->inputVolume();
}

int PhoneCallsModel::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int PhoneCallsModel::outputVolume() const
{
    QList<SipCall*> calls = activeCalls();
    return calls.isEmpty() ? 0 : calls.first()->outputVolume();
}

/** Returns the user's own phone number.
 */
QString PhoneCallsModel::phoneNumber() const
{
    return m_phoneNumber;
}

/** Removes \a count rows starting at the given \a row under the given \a parent.
 *
 * @param row
 * @param count
 * @param parent
 */
bool PhoneCallsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    int last = row + count - 1;
    if (parent.isValid() || row < 0 || count < 0 || last >= m_items.size())
        return false;

    beginRemoveRows(parent, row, last);
    for (int i = last; i >= row; --i) {
        PhoneCallsItem *item = m_items[i];
        QUrl url = m_url;
        url.setPath(url.path() + QString::number(item->id) + "/");
        m_network->deleteResource(buildRequest(url));
        delete item;
        m_items.removeAt(i);
    }
    endRemoveRows();
    return true;
}

/** Returns the number of rows under the given \a parent.
 *
 * @param parent
 */
int PhoneCallsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

/** Returns the selfcare URL.
 */
QUrl PhoneCallsModel::selfcareUrl() const
{
    return m_selfcareUrl;
}

/** Starts sending a tone.
 *
 * @param tone
 */
void PhoneCallsModel::startTone(QXmppRtpAudioChannel::Tone tone)
{
    foreach (SipCall *call, activeCalls())
        call->audioChannel()->startTone(tone);
}

/** Stops sending a tone.
 *
 * @param tone
 */
void PhoneCallsModel::stopTone(QXmppRtpAudioChannel::Tone tone)
{
    foreach (SipCall *call, activeCalls())
        call->audioChannel()->stopTone(tone);
}

/** Requests VoIP settings from the server.
 */
void PhoneCallsModel::_q_getSettings()
{
    qDebug("fetching phone settings");
    QNetworkRequest req(QUrl("https://www.wifirst.net/wilink/voip"));
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    QNetworkReply *reply = m_network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(_q_handleSettings()));
}

void PhoneCallsModel::_q_handleSettings()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("failed to retrieve phone settings: %s", qPrintable(reply->errorString()));
        // retry in 5mn
        m_timer->start(300000);
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
    const QUrl selfcareUrl = QUrl(settings.firstChildElement("selfcare-url").text());

    // update phone number
    if (number != m_phoneNumber) {
        m_phoneNumber = number;
        emit phoneNumberChanged(m_phoneNumber);
    }

    // update selfcare url
    if (selfcareUrl != m_selfcareUrl) {
        m_selfcareUrl = selfcareUrl;
        emit selfcareUrlChanged(m_selfcareUrl);
    }

    // check service is activated
    const bool wasEnabled = m_enabled;
    if (enabled && !domain.isEmpty() && !username.isEmpty() && !password.isEmpty()) {
        // connect to SIP server
        if (m_client->displayName() != number ||
            m_client->domain() != domain ||
            m_client->username() != username ||
            m_client->password() != password)
        {
            m_client->setDisplayName(number);
            m_client->setDomain(domain);
            m_client->setUsername(username);
            m_client->setPassword(password);
            QMetaObject::invokeMethod(m_client, "connectToServer");
        }

        // retrieve call history
        if (!callsUrl.isEmpty()) {
            m_url = callsUrl;

            QNetworkReply *reply = m_network->get(buildRequest(m_url));
            connect(reply, SIGNAL(finished()), this, SLOT(_q_handleList()));
        }

        // register URL handler
        if (!m_registeredHandler) {
            ChatMessage::addTransform(QRegExp("^(.*\\s)?(\\+?[0-9]{4,})(\\s.*)?$"),
                QString("\\1<a href=\"sip:\\2@%1\">\\2</a>\\3").arg(m_client->domain()));
            QDesktopServices::setUrlHandler("sip", this, "_q_openUrl");
            m_registeredHandler = true;
        }

        m_enabled = true;
    } else { 
        m_enabled = false;
    }
    if (m_enabled != wasEnabled)
        emit enabledChanged(m_enabled);
}

void PhoneCallsModel::_q_handleCreate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    // find the item
    PhoneCallsItem *item = 0;
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->reply == reply) {
            item = m_items[i];
            item->reply = 0;
            row = i;
            break;
        }
    }
    if (!item)
        return;

    // update the item
    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to create phone call: %s", qPrintable(reply->errorString()));
        return;
    }
    item->parse(doc.documentElement());
    emit dataChanged(createIndex(row, 0), createIndex(row, 0));
}

void PhoneCallsModel::_q_handleList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to retrieve phone calls: %s", qPrintable(reply->errorString()));
        return;
    }

    QDomElement callElement = doc.documentElement().firstChildElement("call");
    while (!callElement.isNull()) {
        int id = callElement.firstChildElement("id").text().toInt();
        foreach (PhoneCallsItem *item, m_items) {
            if (item->id == id) {
                id = 0;
                break;
            }
        }
        if (id > 0) {
            PhoneCallsItem *item = new PhoneCallsItem;
            item->parse(callElement);

            beginInsertRows(QModelIndex(), 0, 0);
            m_items.prepend(item);
            endInsertRows();
        }
        callElement = callElement.nextSiblingElement("call");
    }
}

void PhoneCallsModel::_q_openUrl(const QUrl &url)
{
    if (url.scheme() != "sip") {
        qWarning("PhoneCallsModel got a non-SIP URL!");
        return;
    }

    if (!url.path().isEmpty()) {
        const QString phoneNumber = url.path().split('@').first();
        const QString recipient = QString("\"%1\" <%2>").arg(phoneNumber, url.toString());
        call(recipient);
    }
    // FIXME
    //emit showPanel();
}

