/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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
#include <QUrl>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"
#include "QXmppRtpChannel.h"
#include "QXmppUtils.h"

#include "declarative.h"
#include "history.h"
#include "phone.h"
#include "phone/sip.h"

#define FLAGS_DIRECTION 0x1
#define FLAGS_ERROR 0x2

class PhoneContactItem
{
public:
    PhoneContactItem();
    QByteArray data() const;
    void parse(const QDomElement &element);

    int id;
    QString name;
    QString phone;
    QNetworkReply *reply;
};

PhoneContactItem::PhoneContactItem()
    : id(0),
    reply(0)
{
}

QByteArray PhoneContactItem::data() const
{
    QUrl data;
    data.addQueryItem("name", name);
    data.addQueryItem("phone", phone);
    return data.encodedQuery();
}

void PhoneContactItem::parse(const QDomElement &element)
{
    id = element.firstChildElement("id").text().toInt();
    name = element.firstChildElement("name").text();
    phone = element.firstChildElement("phone").text();
}

PhoneContactModel::PhoneContactModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(IdRole, "id");
    roleNames.insert(NameRole, "name");
    roleNames.insert(PhoneRole, "phone");
    setRoleNames(roleNames);

    // http
    m_network = new AuthenticatedNetworkAccessManager(this);
}

void PhoneContactModel::addContact(const QString &name, const QString &phone)
{
    PhoneContactItem *item = new PhoneContactItem;
    item->name = name;
    item->phone = phone;

    QNetworkRequest request(m_url);
    request.setRawHeader("Accept", "application/xml");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    item->reply = m_network->post(request, item->data());
    connect(item->reply, SIGNAL(finished()),
            this, SLOT(_q_handleCreate()));

    beginInsertRows(QModelIndex(), 0, 0);
    m_items.prepend(item);
    endInsertRows();

    emit nameChanged(item->phone);
}

/** Returns the number of columns under the given \a parent.
 *
 * @param parent
 */
int PhoneContactModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant PhoneContactModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row > m_items.size())
        return QVariant();

    PhoneContactItem *item = m_items.at(row);
    switch (role) {
    case IdRole:
        return item->id;
    case NameRole:
        return item->name;
    case PhoneRole:
        return item->phone;
    default:
        return QVariant();
    }
}

/** Returns the contact with the given \a id.
 *
 * @param id
 */
QVariant PhoneContactModel::getContact(int id)
{
    for (int row = 0; row < m_items.size(); ++row) {
        QModelIndex idx = index(row, 0);
        if (idx.data(IdRole).toInt() == id) {
            QVariantMap result;
            const QHash<int, QByteArray> names = roleNames();
            foreach (int role, names.keys())
                result.insert(QString::fromAscii(names[role]), idx.data(role));
            result.insert("index", row);
            return result;
        }
    }
    return QVariant();
}

/** Returns the contact with the given \a phone.
 *
 * @param phone
 */
QVariant PhoneContactModel::getContactByPhone(const QString &phone)
{
    for (int row = 0; row < m_items.size(); ++row) {
        QModelIndex idx = index(row, 0);
        if (idx.data(PhoneRole).toString() == phone) {
            QVariantMap result;
            const QHash<int, QByteArray> names = roleNames();
            foreach (int role, names.keys())
                result.insert(QString::fromAscii(names[role]), idx.data(role));
            result.insert("index", row);
            return result;
        }
    }
    return QVariant();
}

/** Removes the contact with the given \a id.
 *
 * @param id
 */
void PhoneContactModel::removeContact(int id)
{
    for (int row = 0; row < m_items.size(); ++row) {
        PhoneContactItem *item = m_items[row];
        if (item->id == id) {
            beginRemoveRows(QModelIndex(), row, row);

            QUrl url = m_url;
            url.setPath(url.path() + QString::number(item->id) + "/");

            QNetworkRequest request(url);
            request.setRawHeader("Accept", "application/xml");
            m_network->deleteResource(request);

            m_items.removeAt(row);
            endRemoveRows();

            emit nameChanged(item->phone);
            delete item;

            break;
        }
    }
}

/** Returns the number of rows under the given \a parent.
 *
 * @param parent
 */
int PhoneContactModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

/** Updates the contact with the given \a id.
 *
 * @param id
 * @param name
 * @param phone
 */
void PhoneContactModel::updateContact(int id, const QString &name, const QString &phone)
{
    for (int row = 0; row < m_items.size(); ++row) {
        PhoneContactItem *item = m_items[row];
        if (item->id == id) {
            const QString oldPhone = item->phone;
            item->name = name;
            item->phone = phone;

            QUrl url = m_url;
            url.setPath(url.path() + QString::number(item->id) + "/");

            QNetworkRequest request(url);
            request.setRawHeader("Accept", "application/xml");
            request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
            m_network->put(request, item->data());

            emit dataChanged(createIndex(row, 0), createIndex(row, 0));
            if (oldPhone != item->phone)
                emit nameChanged(oldPhone);
            emit nameChanged(item->phone);
            break;
        }
    }
}

QString PhoneContactModel::name(const QString &number) const
{
    foreach (PhoneContactItem *item, m_items) {
        if (number == item->phone)
            return item->name;
    }
    return QString();
}

QUrl PhoneContactModel::url() const
{
    return m_url;
}

void PhoneContactModel::setUrl(const QUrl &url)
{
    if (url != m_url) {
        m_url = url;
        if (m_url.isValid()) {
            QNetworkRequest request(m_url);
            request.setRawHeader("Accept", "application/xml");
            QNetworkReply *reply = m_network->get(request);
            connect(reply, SIGNAL(finished()),
                    this, SLOT(_q_handleList()));
        }
        emit urlChanged(m_url);
    }
}

void PhoneContactModel::_q_handleCreate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    // find the item
    PhoneContactItem *item = 0;
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
        qWarning("Failed to create phone contact: %s", qPrintable(reply->errorString()));
        return;
    }
    item->parse(doc.documentElement());
    emit dataChanged(createIndex(row, 0), createIndex(row, 0));
}

void PhoneContactModel::_q_handleList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to retrieve phone contacts: %s", qPrintable(reply->errorString()));
        return;
    }

    QDomElement element = doc.documentElement().firstChildElement("contact");
    while (!element.isNull()) {
        int id = element.firstChildElement("id").text().toInt();
        foreach (PhoneContactItem *item, m_items) {
            if (item->id == id) {
                id = 0;
                break;
            }
        }
        if (id > 0) {
            PhoneContactItem *item = new PhoneContactItem;
            item->parse(element);

            beginInsertRows(QModelIndex(), 0, 0);
            m_items.prepend(item);
            endInsertRows();

            emit nameChanged(item->phone);
        }
        element = element.nextSiblingElement("contact");
    }
}

class PhoneHistoryItem
{
public:
    PhoneHistoryItem();
    QByteArray data() const;
    QString number() const;
    void parse(const QDomElement &element);

    int id;
    QString address;
    QDateTime date;
    int duration;
    int flags;

    QSoundStream *audioStream;
    SipCall *call;
    QNetworkReply *reply;
    QSoundPlayerJob *soundJob;
};

PhoneHistoryItem::PhoneHistoryItem()
    : id(0),
    duration(0),
    flags(0),
    audioStream(0),
    call(0),
    reply(0),
    soundJob(0)
{
}

QByteArray PhoneHistoryItem::data() const
{
    QUrl data;
    data.addQueryItem("address", address);
    data.addQueryItem("duration", QString::number(duration));
    data.addQueryItem("flags", QString::number(flags));
    return data.encodedQuery();
}

QString PhoneHistoryItem::number() const
{
    const QString uri = sipAddressToUri(address);
    const QStringList bits = uri.split('@');
    if (bits.size() == 2) {
        return bits[0].mid(4);
    }
    return QString();
}

void PhoneHistoryItem::parse(const QDomElement &element)
{
    id = element.firstChildElement("id").text().toInt();
    address = element.firstChildElement("address").text();
    date = QXmppUtils::datetimeFromString(element.firstChildElement("date").text());
    duration = element.firstChildElement("duration").text().toInt();
    flags = element.firstChildElement("flags").text().toInt();
}

/** Constructs a new model representing the call history.
 *
 * @param network
 * @param parent
 */
PhoneHistoryModel::PhoneHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_registeredHandler(false)
{
    bool check;
    Q_UNUSED(check);

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ActiveRole, "active");
    roleNames.insert(AddressRole, "address");
    roleNames.insert(DateRole, "date");
    roleNames.insert(DirectionRole, "direction");
    roleNames.insert(DurationRole, "duration");
    roleNames.insert(IdRole, "id");
    roleNames.insert(NameRole, "name");
    roleNames.insert(PhoneRole, "phone");
    roleNames.insert(StateRole, "state");
    setRoleNames(roleNames);

    // contacts
    m_contactsModel = new PhoneContactModel(this);
    connect(m_contactsModel, SIGNAL(nameChanged(QString)),
            this, SLOT(_q_nameChanged(QString)));

    // http
    m_network = new AuthenticatedNetworkAccessManager(this);

    // sound
    m_player = QSoundPlayer::instance();

    // sip
    m_client = new SipClient;
    check = connect(m_client, SIGNAL(callDialled(SipCall*)),
                    this, SLOT(addCall(SipCall*)));
    Q_ASSERT(check);
    m_client->moveToThread(m_player->soundThread());

    // ticker for call durations
    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, SIGNAL(timeout()),
            this, SLOT(callTick()));

    // register URL handler
    if (!m_registeredHandler) {
        HistoryMessage::addTransform(QRegExp("^(.*\\s)?(\\+?[0-9]{4,})(\\s.*)?$"),
            QString("\\1<a href=\"sip:\\2@%1\">\\2</a>\\3").arg(m_client->domain()));
        QDesktopServices::setUrlHandler("sip", this, "_q_openUrl");
        m_registeredHandler = true;
    }
}

PhoneHistoryModel::~PhoneHistoryModel()
{
    m_ticker->stop();

    // try to exit SIP client cleanly
    if (m_client->state() == SipClient::ConnectedState)
        QMetaObject::invokeMethod(m_client, "disconnectFromServer");
    m_client->deleteLater();

    // delete items
    foreach (PhoneHistoryItem *item, m_items)
        delete item;
}

QList<SipCall*> PhoneHistoryModel::activeCalls() const
{
    QList<SipCall*> calls;
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            calls << m_items[i]->call;
    return calls;
}

void PhoneHistoryModel::addCall(SipCall *call)
{
    PhoneHistoryItem *item = new PhoneHistoryItem;
    item->address = call->recipient();
    item->flags = call->direction();
    item->call = call;
    connect(item->call, SIGNAL(stateChanged(SipCall::State)),
            this, SLOT(callStateChanged(SipCall::State)));
    if (item->call->direction() == SipCall::OutgoingDirection)
        connect(item->call, SIGNAL(ringing()), this, SLOT(callRinging()));

    QNetworkRequest request(m_url);
    request.setRawHeader("Accept", "application/xml");
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

bool PhoneHistoryModel::call(const QString &address)
{
    if (m_client->state() == SipClient::ConnectedState &&
        !currentCalls()) {
        QMetaObject::invokeMethod(m_client, "call", Q_ARG(QString, address));
        return true;
    }
    return false;
}

void PhoneHistoryModel::callRinging()
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // find the call
    foreach (PhoneHistoryItem *item, m_items) {
        if (item->call == call && !item->soundJob) {
            item->soundJob = m_player->play(QUrl(":/sounds/call-outgoing.ogg"), true);
            break;
        }
    }
}

void PhoneHistoryModel::callStateChanged(SipCall::State state)
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

    PhoneHistoryItem *item = m_items[row];

    if (item->soundJob && state != SipCall::ConnectingState) {
        item->soundJob->stop();
        item->soundJob = 0;
    }

    // update the item
    if (state == SipCall::ActiveState) {

        // start audio input / output
        if (!item->audioStream) {
            QXmppRtpAudioChannel *channel = item->call->audioChannel();

            item->audioStream = new QSoundStream(m_player);
            item->audioStream->setDevice(channel);
            item->audioStream->setFormat(
                channel->payloadType().channels(),
                channel->payloadType().clockrate());

            connect(item->audioStream, SIGNAL(inputVolumeChanged(int)),
                    this, SIGNAL(inputVolumeChanged(int)));
            connect(item->audioStream, SIGNAL(outputVolumeChanged(int)),
                    this, SIGNAL(outputVolumeChanged(int)));

            item->audioStream->moveToThread(m_player->soundThread());
            QMetaObject::invokeMethod(item->audioStream, "startOutput");
            QMetaObject::invokeMethod(item->audioStream, "startInput");
        }

    } else if (state == SipCall::FinishedState) {
        // stop audio input / output
        if (item->audioStream) {
            item->audioStream->stopInput();
            item->audioStream->stopOutput();
            delete item->audioStream;
            item->audioStream = 0;
        }

        call->disconnect(this);
        item->call = 0;
        item->duration = call->duration();
        if (!call->errorString().isEmpty()) {
            item->flags |= FLAGS_ERROR;
            emit error(call->errorString());
        }
        emit currentCallsChanged();
        emit inputVolumeChanged(0);
        emit outputVolumeChanged(0);

        QUrl url = m_url;
        url.setPath(url.path() + QString::number(item->id) + "/");

        QNetworkRequest request(url);
        request.setRawHeader("Accept", "application/xml");
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        m_network->put(request, item->data());

        // FIXME: ugly workaround for a race condition causing a crash in
        // PhoneNotification.qml
        QTimer::singleShot(1000, call, SLOT(deleteLater()));
    }
    emit dataChanged(createIndex(item), createIndex(item));
}

void PhoneHistoryModel::callTick()
{
    bool active = false;
    foreach (PhoneHistoryItem *item, m_items) {
        if (item->call) {
            active = true;
            emit dataChanged(createIndex(item), createIndex(item));
        }
    }
    if (!active)
        m_ticker->stop();
}

void PhoneHistoryModel::clear()
{
    foreach (PhoneHistoryItem *item, m_items)
        removeCall(item->id);
}

/** Returns the underlying SIP client.
 *
 * \note Use with care as the SIP client lives in a different thread!
 */
SipClient *PhoneHistoryModel::client() const
{
    return m_client;
}

/** Returns the number of columns under the given \a parent.
 *
 * @param parent
 */
int PhoneHistoryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

PhoneContactModel* PhoneHistoryModel::contactsModel() const
{
    return m_contactsModel;
}

QModelIndex PhoneHistoryModel::createIndex(PhoneHistoryItem *item)
{
    const int row = m_items.indexOf(item);
    if (row >= 0)
        return QAbstractItemModel::createIndex(row, 0);
    return QModelIndex();
}

int PhoneHistoryModel::currentCalls() const
{
    return activeCalls().size();
}

QVariant PhoneHistoryModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row > m_items.size())
        return QVariant();
    PhoneHistoryItem *item = m_items[row];

    switch (role) {
    case Qt::ToolTipRole:
        return item->address;
    case AddressRole:
        return item->address;
    case ActiveRole:
        return item->call != 0;
    case DateRole:
        return item->date;
    case DirectionRole:
        return item->flags & FLAGS_DIRECTION;
    case DurationRole:
        return item->call ? item->call->duration() : item->duration;
    case IdRole:
        return item->id;
    case NameRole: {
        const QString name = m_contactsModel->name(item->number());
        if (!name.isEmpty())
            return name;
        else
            return sipAddressToName(item->address);
    }
    case PhoneRole:
        return item->number();
    default:
        return QVariant();
    }
}

/** Hangs up all active calls.
 */
void PhoneHistoryModel::hangup()
{
    for (int i = m_items.size() - 1; i >= 0; --i)
        if (m_items[i]->call)
            m_items[i]->call->hangup();
}

int PhoneHistoryModel::inputVolume() const
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i]->audioStream)
            return m_items[i]->audioStream->inputVolume();
    }
    return 0;
}

int PhoneHistoryModel::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int PhoneHistoryModel::outputVolume() const
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i]->audioStream)
            return m_items[i]->audioStream->outputVolume();
    }
    return 0;
}

/** Removes the call with the given \a id.
 *
 * @param id
 */
void PhoneHistoryModel::removeCall(int id)
{
    for (int row = 0; row < m_items.size(); ++row) {
        PhoneHistoryItem *item = m_items[row];
        if (item->id == id) {
            // removing active calls is not allowed
            if (item->call)
                return;

            beginRemoveRows(QModelIndex(), row, row);

            QUrl url = m_url;
            url.setPath(url.path() + QString::number(item->id) + "/");

            QNetworkRequest request(url);
            request.setRawHeader("Accept", "application/xml");
            m_network->deleteResource(request);
            m_items.removeAt(row);
            delete item;

            endRemoveRows();
            break;
        }
    }
}

/** Returns the number of rows under the given \a parent.
 *
 * @param parent
 */
int PhoneHistoryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_items.size();
}

/** Starts sending a tone.
 *
 * @param tone
 */
void PhoneHistoryModel::startTone(int toneValue)
{
    if (toneValue < QXmppRtpAudioChannel::Tone_0 || toneValue > QXmppRtpAudioChannel::Tone_D)
        return;
    QXmppRtpAudioChannel::Tone tone = static_cast<QXmppRtpAudioChannel::Tone>(toneValue);
    foreach (SipCall *call, activeCalls()) {
        call->audioChannel()->startTone(tone);
    }
}

/** Stops sending a tone.
 *
 * @param tone
 */
void PhoneHistoryModel::stopTone(int toneValue)
{
    if (toneValue < QXmppRtpAudioChannel::Tone_0 || toneValue > QXmppRtpAudioChannel::Tone_D)
        return;
    QXmppRtpAudioChannel::Tone tone = static_cast<QXmppRtpAudioChannel::Tone>(toneValue);
    foreach (SipCall *call, activeCalls())
        call->audioChannel()->stopTone(tone);
}

QUrl PhoneHistoryModel::url() const
{
    return m_url;
}

void PhoneHistoryModel::setUrl(const QUrl &url)
{
    if (url != m_url) {
        m_url = url;

        QNetworkRequest request(m_url);
        request.setRawHeader("Accept", "application/xml");
        QNetworkReply *reply = m_network->get(request);
        connect(reply, SIGNAL(finished()), this, SLOT(_q_handleList()));

        emit urlChanged(m_url);
    }
}

void PhoneHistoryModel::_q_handleCreate()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    // find the item
    PhoneHistoryItem *item = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->reply == reply) {
            item = m_items[i];
            item->reply = 0;
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
    emit dataChanged(createIndex(item), createIndex(item));
}

void PhoneHistoryModel::_q_handleList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(reply);

    QDomDocument doc;
    if (reply->error() != QNetworkReply::NoError || !doc.setContent(reply)) {
        qWarning("Failed to retrieve phone calls: %s", qPrintable(reply->errorString()));
        return;
    }

    QDomElement element = doc.documentElement().firstChildElement("call");
    while (!element.isNull()) {
        int id = element.firstChildElement("id").text().toInt();
        foreach (PhoneHistoryItem *item, m_items) {
            if (item->id == id) {
                id = 0;
                break;
            }
        }
        if (id > 0) {
            PhoneHistoryItem *item = new PhoneHistoryItem;
            item->parse(element);

            beginInsertRows(QModelIndex(), 0, 0);
            m_items.prepend(item);
            endInsertRows();
        }
        element = element.nextSiblingElement("call");
    }
}

void PhoneHistoryModel::_q_nameChanged(const QString &phone)
{
    foreach (PhoneHistoryItem *item, m_items) {
        if (item->number() == phone)
            emit dataChanged(createIndex(item), createIndex(item));
    }
}

void PhoneHistoryModel::_q_openUrl(const QUrl &url)
{
    if (url.scheme() != "sip") {
        qWarning("PhoneHistoryModel got a non-SIP URL!");
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


