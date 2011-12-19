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

#include <QUrl>

#include "QXmppArchiveIq.h"
#include "QXmppArchiveManager.h"
#include "QXmppUtils.h"

#include "client.h"
#include "history.h"
#include "roster.h"

#ifdef WILINK_EMBEDDED
#define HISTORY_DAYS 7
#else
#define HISTORY_DAYS 14
#endif

typedef QPair<QRegExp, QString> TextTransform;
static QList<TextTransform> textTransforms;

static const QRegExp meRegex = QRegExp("^/me[ \t]+(.*)");

class HistoryItem : public ChatModelItem
{
public:
    HistoryItem() : selected(false) {}
    ~HistoryItem();

    QList<HistoryMessage*> messages;
    bool selected;
};

HistoryItem::~HistoryItem()
{
    foreach (HistoryMessage *message, messages)
        delete message;
}

/** Constructs a new HistoryMessage.
 */
HistoryMessage::HistoryMessage()
    : archived(false), received(true)
{
}

/** Adds a transformation to be applied to message bodies.
 *
 * @param match
 * @param replacement
 */
void HistoryMessage::addTransform(const QRegExp &match, const QString &replacement)
{
    textTransforms.append(qMakePair(match, replacement));
}

/** Returns true if the two messages should be grouped in the same bubble.
 *
 * @param other
 */
bool HistoryMessage::groupWith(const HistoryMessage &other) const
{
    return jid == other.jid &&
        !isAction() &&
        !other.isAction() &&
        qAbs(date.secsTo(other.date)) < 3600; // 1 hour
}

static QString transformToken(const QString &token)
{
    QMap<QString, QString> smileys;
    smileys.insert(":@", ":/face-angry.png");
    smileys.insert(":-@", ":/face-angry.png");
    smileys.insert(":s", ":/face-uncertain.png");
    smileys.insert(":-s", ":/face-uncertain.png");
    smileys.insert(":S", ":/face-uncertain.png");
    smileys.insert(":-S", ":/face-uncertain.png");
    smileys.insert(":)", ":/face-smile.png");
    smileys.insert(":-)", ":/face-smile.png");
    smileys.insert(":|", ":/face-plain.png");
    smileys.insert(":-|", ":/face-plain.png");
    smileys.insert(":p", ":/face-raspberry.png");
    smileys.insert(":-p", ":/face-raspberry.png");
    smileys.insert(":P", ":/face-raspberry.png");
    smileys.insert(":-P", ":/face-raspberry.png");
    smileys.insert(":(", ":/face-sad.png");
    smileys.insert(":-(", ":/face-sad.png");
    smileys.insert(";)", ":/face-wink.png");
    smileys.insert(";-)", ":/face-wink.png");

    const QRegExp linkRegex("(ftp|http|https)://.+");

    // handle smileys
    foreach (const QString &key, smileys.keys()) {
        if (token == key)
            return QString("<img alt=\"%1\" src=\"%2\" />").arg(key, smileys.value(token));
    }

    // handle links
    if (linkRegex.exactMatch(token)) {
        QUrl url;
        url.setEncodedUrl(token.toAscii());
        return QString("<a href=\"%1\">%2</a>").arg(url.toString(), url.toString());
    }

    // default
    return Qt::escape(token);
}

/** Returns the HTML for the message body.
 *
 * @param meName
 */
QString HistoryMessage::html(const QString &meName) const
{
    // me
    QRegExp re(meRegex);
    if (!meName.isEmpty() && re.exactMatch(body))
        return QString("<b>%1 %2</b>").arg(meName, Qt::escape(re.cap(1)));

    const QStringList input = body.split(QRegExp("[ \t]+"));
    QStringList output;
    foreach (const QString &token, input) {
        QString tokenHtml = transformToken(token);
        foreach (const TextTransform &transform, textTransforms)
            tokenHtml.replace(transform.first, transform.second);
        output << tokenHtml;
    }

    QString bodyHtml = output.join(" ");
    bodyHtml.replace("\n", "<br/>");
    return bodyHtml;
}

/** Returns true if the message is an "action" message, such
 *  as /me.
 */
bool HistoryMessage::isAction() const
{
    return meRegex.exactMatch(body);
}

class HistoryModelPrivate
{
public:
    HistoryModelPrivate();
    void fetchArchives();

    bool archivesFetched;
    ChatClient *client;
    QString jid;
    QMap<QString, VCard*> rosterCards;
};

HistoryModelPrivate::HistoryModelPrivate()
    : archivesFetched(false),
    client(0)
{
}

void HistoryModelPrivate::fetchArchives()
{
    if (archivesFetched || !client || jid.isEmpty())
        return;

    client->archiveManager()->listCollections(jid,
        client->serverTime().addDays(-HISTORY_DAYS));
    archivesFetched = true;
}

/** Constructs a new HistoryModel.
 *
 * @param parent
 */
HistoryModel::HistoryModel(QObject *parent)
    : ChatModel(parent)
{
    d = new HistoryModelPrivate;

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ActionRole, "action");
    roleNames.insert(AvatarRole, "avatar");
    roleNames.insert(BodyRole, "body");
    roleNames.insert(DateRole, "date");
    roleNames.insert(FromRole, "from");
    roleNames.insert(HtmlRole, "html");
    roleNames.insert(JidRole, "jid");
    roleNames.insert(ReceivedRole, "received");
    roleNames.insert(SelectedRole, "selected");
    setRoleNames(roleNames);
}

/** Adds a message in the chat history.
 *
 * The message is inserted in chronological order and grouped with
 * related messages (adjacent messages from the same sender, which
 * are not "actions" such as /me, and which are less than 1 hour apart).
 *
 * \note Archived messages are checked for duplicates.
 *
 * @param message
 */
void HistoryModel::addMessage(const HistoryMessage &message)
{
    if (message.body.isEmpty())
        return;

    // position cursor
    HistoryItem *prevBubble = 0;
    HistoryItem *nextBubble = 0;
    HistoryMessage *prevMsg = 0;
    HistoryMessage *nextMsg = 0;
    foreach (ChatModelItem *bubblePtr, rootItem->children) {
        HistoryItem *bubble = static_cast<HistoryItem*>(bubblePtr);
        foreach (HistoryMessage *curMessage, bubble->messages) {

            // check for duplicate
            if (message.archived &&
                message.jid == curMessage->jid &&
                message.body == curMessage->body &&
                message.date.msecsTo(curMessage->date) < 2000)
                return;

            // we use greater or equal comparison (and not strictly greater) dates
            // because messages are usually received in chronological order
            if (message.date >= curMessage->date) {
                prevBubble = bubble;
                prevMsg = curMessage;
            } else if (!nextMsg) {
                nextBubble = bubble;
                nextMsg = curMessage;
            }
        }
    }

    // prepare message
    HistoryMessage *msg = new HistoryMessage(message);

    // notify bottom is about to change
    emit bottomAboutToChange();

    if (prevMsg && prevMsg->groupWith(message) &&
        nextMsg && nextMsg->groupWith(message) &&
        prevBubble != nextBubble) {
        // message belongs both to the previous and next bubble, merge
        int row = prevBubble->messages.indexOf(prevMsg) + 1;
        prevBubble->messages.insert(row++, msg);
        const int lastRow = nextBubble->messages.size() - 1;
        for (int i = lastRow; i >= 0; --i) {
            HistoryMessage *item = nextBubble->messages.takeAt(i);
            prevBubble->messages.insert(row, item);
        }
        removeRow(nextBubble->row());
        emit dataChanged(createIndex(prevBubble), createIndex(prevBubble));
    }
    else if (prevMsg && prevMsg->groupWith(message))
    {
        // message belongs to the same bubble as previous message
        const int row = prevBubble->messages.indexOf(prevMsg) + 1;
        prevBubble->messages.insert(row, msg);
        emit dataChanged(createIndex(prevBubble), createIndex(prevBubble));
    }
    else if (nextMsg && nextMsg->groupWith(message))
    {
        // message belongs to the same bubble as next message
        const int row = nextBubble->messages.indexOf(nextMsg);
        nextBubble->messages.insert(row, msg);
        emit dataChanged(createIndex(nextBubble), createIndex(nextBubble));
    }
    else
    {
        // split the previous bubble if needed
        int bubblePos = 0;
        if (prevMsg) {
            bubblePos = prevBubble->row() + 1;

            // if the previous message is not the last in its bubble, split
            const int index = prevBubble->messages.indexOf(prevMsg);
            const int lastRow = prevBubble->messages.size() - 1;
            if (index < lastRow) {
                HistoryItem *bubble = new HistoryItem;
                const int firstRow = index + 1;
                for (int i = lastRow; i >= firstRow; --i) {
                    HistoryMessage *item = prevBubble->messages.takeAt(i);
                    bubble->messages.prepend(item);
                }
                addItem(bubble, rootItem, bubblePos);
            }
        }

        // insert the new bubble
        HistoryItem *bubble = new HistoryItem;
        bubble->messages.append(msg);
        addItem(bubble, rootItem, bubblePos);
    }

    // notify bottom change
    emit bottomChanged();
    if (msg->received && !msg->archived)
        emit messageReceived(msg->jid, msg->body);
}

/** Clears all messages.
 */
void HistoryModel::clear()
{
    // clear model
    int rows = rowCount(QModelIndex());
    if (rows > 0)
        removeRows(0, rows);

    // clear archives
    if (d->client && !d->jid.isEmpty())
        d->client->archiveManager()->removeCollections(d->jid);
}

ChatClient *HistoryModel::client() const
{
    return d->client;
}

void HistoryModel::setClient(ChatClient *client)
{
    bool check;
    Q_UNUSED(check);

    if (client == d->client)
        return;

    if (client) {
        check = connect(client->archiveManager(), SIGNAL(archiveChatReceived(QXmppArchiveChat)),
                        this, SLOT(_q_archiveChatReceived(QXmppArchiveChat)));
        Q_ASSERT(check);

        check = connect(client->archiveManager(), SIGNAL(archiveListReceived(QList<QXmppArchiveChat>)),
                        this, SLOT(_q_archiveListReceived(QList<QXmppArchiveChat>)));
        Q_ASSERT(check);
    }

    d->client = client;
    emit clientChanged(d->client);

    // try to fetch archives
    d->fetchArchives();
}

QString HistoryModel::jid() const
{
    return d->jid;
}

void HistoryModel::setJid(const QString &jid)
{
    if (jid != d->jid) {
        d->jid = jid;
        emit jidChanged(d->jid);

        // try to fetch archives
        d->fetchArchives();
    }
}

/** Returns the number of columns for the children of the given parent.
 *
 * @param parent
 */
int HistoryModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

/** Returns the data stored under the given \a role for the item referred to
 *  by the \a index.
 *
 * @param index
 * @param role
 */
QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    HistoryItem *item = static_cast<HistoryItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    HistoryMessage *msg = item->messages.first();
    if (role == ActionRole) {
        return msg->isAction();
    } else if (role == AvatarRole) {
        return VCardCache::instance()->imageUrl(msg->jid);
    } else if (role == BodyRole) {
        QStringList bodies;
        foreach (HistoryMessage *ptr, item->messages)
            bodies << ptr->body;
        return bodies.join("\n");
    } else if (role == DateRole) {
        return msg->date;
    } else if (role == FromRole) {
        const QString jid = msg->jid;

        // chat rooms
        const QString resource = jidToResource(jid);
        if (!resource.isEmpty())
            return resource;

        // conversations
        if (!d->rosterCards.contains(jid)) {
            VCard *card = new VCard((QObject*)this);
            card->setJid(jid);
            connect(card, SIGNAL(nameChanged(QString)),
                    this, SLOT(_q_cardChanged()));
            d->rosterCards.insert(jid, card);
        }
        return d->rosterCards.value(jid)->name();
    } else if (role == HtmlRole) {
        const QString meName = data(index, FromRole).toString();
        QString bodies;
        foreach (HistoryMessage *ptr, item->messages)
            bodies += "<p style=\"margin-top: 0; margin-bottom: 2\">" + ptr->html(meName) + "</p>";
        return bodies;
    } else if (role == JidRole) {
        return msg->jid;
    } else if (role == ReceivedRole) {
        return msg->received;
    } else if (role == SelectedRole) {
        return item->selected;
    }

    return QVariant();
}

void HistoryModel::select(int from, int to)
{
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);

    int i = 0;
    foreach (ChatModelItem *it, rootItem->children) {
        HistoryItem *item = static_cast<HistoryItem*>(it);
        const bool selected = (i >= lo && i <= hi);
        if (selected != item->selected) {
            item->selected = selected;
            emit dataChanged(createIndex(item), createIndex(item));
        }
        ++i;
    }
}

void HistoryModel::_q_cardChanged()
{
    VCard *card = qobject_cast<VCard*>(sender());
    if (!card)
        return;
    const QString jid = card->jid();
    foreach (ChatModelItem *it, rootItem->children) {
        HistoryItem *item = static_cast<HistoryItem*>(it);
        if (item->messages.first()->jid == jid)
            emit dataChanged(createIndex(it), createIndex(it));
    }
}

void HistoryModel::_q_archiveChatReceived(const QXmppArchiveChat &chat)
{
    Q_ASSERT(d->client);

    if (jidToBareJid(chat.with()) != d->jid)
        return;

    foreach (const QXmppArchiveMessage &msg, chat.messages()) {
        HistoryMessage message;
        message.archived = true;
        message.body = msg.body();
        message.date = msg.date();
        message.jid = msg.isReceived() ? jidToBareJid(chat.with()) : d->client->configuration().jidBare();
        message.received = msg.isReceived();
        addMessage(message);
    }
}

void HistoryModel::_q_archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    Q_ASSERT(d->client);

    for (int i = chats.size() - 1; i >= 0; i--)
        if (jidToBareJid(chats[i].with()) == d->jid)
            d->client->archiveManager()->retrieveCollection(chats[i].with(), chats[i].start());
}

