/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#define HISTORY_DAYS 365
#define HISTORY_PAGE 2
#ifdef WILINK_EMBEDDED
#define SMILEY_ROOT ":/images/32x32"
#else
#define SMILEY_ROOT ":/images/16x16"
#endif

typedef QPair<QRegExp, QString> TextTransform;
static QList<TextTransform> textTransforms;

static const QRegExp meRegex = QRegExp("^/me[ \t]+(.*)");

enum PageDirection {
    PageForwards = 0,
    PageBackwards
};

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
    QMap<QString, QStringList> smileys;
    smileys["face-angry.png"] = QStringList() << ":@" << ":-@";
    smileys["face-cool.png"] = QStringList() << "8-)" << "B-)";
    smileys["face-crying.png"] = QStringList() << ";(" << ";-(" << ";'-(" << ":'(" << ":'-(";
    smileys["face-embarrassed.png"] = QStringList() << ":$";
    smileys["face-laughing.png"] = QStringList() << ":D" << ":-D";
    smileys["face-plain.png"] = QStringList() << ":|" << ":-|";
    smileys["face-raspberry.png"] = QStringList() << ":p" << ":-p" << ":P" << ":-P";
    smileys["face-sad.png"] = QStringList() << ":(" << ":-(";
    smileys["face-sleeping.png"] = QStringList() << "|-)";
    smileys["face-smile.png"] = QStringList() << ":)" << ":-)";
    smileys["face-surprise.png"] = QStringList() << ":o" << ":-o" << ":O" << ":-O";
    smileys["face-uncertain.png"] = QStringList() << ":s" << ":-s" << ":S" << ":-S" << ":/" << ":-/";
    smileys["face-wink.png"] = QStringList() << ";)" << ";-)";

    const QRegExp linkRegex("(ftp|http|https)://.+");

    // handle smileys
    foreach (const QString &smiley, smileys.keys()) {
        if (smileys.value(smiley).contains(token))
            return QString("<img alt=\"%1\" src=\"%2/%3\" />").arg(token, SMILEY_ROOT, smiley);
    }

    // handle links
    if (linkRegex.exactMatch(token)) {
        QUrl url;
        url.setUrl(token);
        return QString("<a href=\"%1\">%2</a>").arg(url.toString(), url.toString());
    }

    // default
    return token.toHtmlEscaped();
}

/** Returns the HTML for the message body.
 *
 * @param meName
 */
QString HistoryMessage::html(const QString &meName) const
{
    // me
    QRegExp re(meRegex);
    if (!meName.isEmpty() && re.exactMatch(body)) {
        const QString meBody = re.cap(1).toHtmlEscaped();
        return QString("<b>%1 %2</b>").arg(meName, meBody);
    }

    // remove trailing whitespace
    int pos = body.size() - 1;
    while (pos >= 0 && body.at(pos).isSpace())
        pos--;

    // transform each line into HTML
    QStringList htmlLines;
    foreach (const QString &textLine, body.left(pos+1).split('\n')) {
        QStringList output;
        const QStringList input = textLine.split(QRegExp("[ \t]+"));
        foreach (const QString &token, input) {
            QString tokenHtml = transformToken(token);
            foreach (const TextTransform &transform, textTransforms)
                tokenHtml.replace(transform.first, transform.second);
            output << tokenHtml;
        }
        htmlLines << output.join(" ");
    }

    return htmlLines.join("<br/>");
}

/** Returns true if the message is an "action" message, such
 *  as /me.
 */
bool HistoryMessage::isAction() const
{
    return meRegex.exactMatch(body);
}

class HistoryQueueItem
{
public:
    QString with;
    QDateTime start;
};

class HistoryModelPrivate
{
public:
    HistoryModelPrivate(HistoryModel *qq);
    void fetchArchives();
    void fetchMessages();

    QString archiveFirst;
    QString archiveLast;
    bool archivesFetched;
    ChatClient *client;
    bool hasPreviousPage;
    QList<HistoryQueueItem> messageQueue;
    PageDirection pageDirection;
    QString jid;
    QMap<QString, VCard*> rosterCards;

private:
    HistoryModel *q;
};

HistoryModelPrivate::HistoryModelPrivate(HistoryModel *qq)
    : archivesFetched(false)
    , client(0)
    , hasPreviousPage(false)
    , pageDirection(PageBackwards)
    , q(qq)
{
}

void HistoryModelPrivate::fetchArchives()
{
    if (archivesFetched || !client || jid.isEmpty())
        return;

    // fetch last page
    archivesFetched = true;
    archiveFirst = QString("");
    messageQueue.clear();
    q->fetchPreviousPage();
}

void HistoryModelPrivate::fetchMessages()
{
    if (messageQueue.isEmpty())
        return;

    HistoryQueueItem item = messageQueue.takeFirst();
    // FIXME: setting max to -2 is a hack!
    client->archiveManager()->retrieveCollection(item.with, item.start, -2);
}

/** Constructs a new HistoryModel.
 *
 * @param parent
 */
HistoryModel::HistoryModel(QObject *parent)
    : ChatModel(parent)
{
    d = new HistoryModelPrivate(this);
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
    // notify bottom is about to change
    emit bottomAboutToChange();

    addMessage_worker(message);

    // notify bottom change
    emit bottomChanged();
}

void HistoryModel::addMessage_worker(const HistoryMessage &message)
{
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
        changeItem(prevBubble);
    }
    else if (prevMsg && prevMsg->groupWith(message))
    {
        // message belongs to the same bubble as previous message
        const int row = prevBubble->messages.indexOf(prevMsg) + 1;
        prevBubble->messages.insert(row, msg);
        changeItem(prevBubble);
    }
    else if (nextMsg && nextMsg->groupWith(message))
    {
        // message belongs to the same bubble as next message
        const int row = nextBubble->messages.indexOf(nextMsg);
        nextBubble->messages.insert(row, msg);
        changeItem(nextBubble);
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

    // notify message
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

/** Fetches the next page.
 */
void HistoryModel::fetchNextPage()
{
    d->pageDirection = PageForwards;
    QXmppResultSetQuery rsmQuery;
    rsmQuery.setAfter(d->archiveLast);
    rsmQuery.setMax(HISTORY_PAGE);
    d->client->archiveManager()->listCollections(d->jid,
        d->client->serverTime().addDays(-HISTORY_DAYS), QDateTime(), rsmQuery);
}

/** Fetches the previous page.
 */
void HistoryModel::fetchPreviousPage()
{
    if (d->hasPreviousPage) {
        d->hasPreviousPage = false;
        emit pagesChanged();
    }

    d->pageDirection = PageBackwards;
    QXmppResultSetQuery rsmQuery;
    rsmQuery.setBefore(d->archiveFirst);
    rsmQuery.setMax(HISTORY_PAGE);
    d->client->archiveManager()->listCollections(d->jid,
        d->client->serverTime().addDays(-HISTORY_DAYS), QDateTime(), rsmQuery);
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
        check = connect(client->archiveManager(), SIGNAL(archiveChatReceived(QXmppArchiveChat,QXmppResultSetReply)),
                        this, SLOT(_q_archiveChatReceived(QXmppArchiveChat,QXmppResultSetReply)));
        Q_ASSERT(check);

        check = connect(client->archiveManager(), SIGNAL(archiveListReceived(QList<QXmppArchiveChat>,QXmppResultSetReply)),
                        this, SLOT(_q_archiveListReceived(QList<QXmppArchiveChat>,QXmppResultSetReply)));
        Q_ASSERT(check);
    }

    d->client = client;
    emit clientChanged(d->client);

    // try to fetch archives
    d->fetchArchives();
}

bool HistoryModel::hasPreviousPage() const
{
    return d->hasPreviousPage;
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
        const QString resource = QXmppUtils::jidToResource(jid);
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

QHash<int, QByteArray> HistoryModel::roleNames() const
{
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
    return roleNames;
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
            changeItem(item);
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
            changeItem(item);
    }
}

void HistoryModel::_q_archiveChatReceived(const QXmppArchiveChat &chat, const QXmppResultSetReply &rsmReply)
{
    Q_UNUSED(rsmReply);
    Q_ASSERT(d->client);

    if (QXmppUtils::jidToBareJid(chat.with()) != d->jid)
        return;

    // notify bottom is about to change
    emit bottomAboutToChange();

    beginBuffering();
    foreach (const QXmppArchiveMessage &msg, chat.messages()) {
        if (msg.body().isEmpty())
            continue;

        HistoryMessage message;
        message.archived = true;
        message.body = msg.body();
        message.date = msg.date();
        message.jid = msg.isReceived() ? QXmppUtils::jidToBareJid(chat.with()) : d->client->configuration().jidBare();
        message.received = msg.isReceived();
        addMessage_worker(message);
    }
    endBuffering();

    // notify bottom change
    emit bottomChanged();

    // process queue
    d->fetchMessages();
}

void HistoryModel::_q_archiveListReceived(const QList<QXmppArchiveChat> &chats, const QXmppResultSetReply &rsmReply)
{
    Q_ASSERT(d->client);

    if (chats.isEmpty() || QXmppUtils::jidToBareJid(chats.first().with()) != d->jid)
        return;

    // update boundaries
    if (d->archiveFirst.isEmpty() || d->pageDirection == PageBackwards)
        d->archiveFirst = rsmReply.first();
    if (d->archiveLast.isEmpty() || d->pageDirection == PageForwards)
        d->archiveLast = rsmReply.last();

    //qDebug("received page %i - %i (of 0 - %i)", rsmReply.index(), rsmReply.index() + chats.size() - 1, rsmReply.count() - 1);
    const bool hasPreviousPage = rsmReply.index() > 0;
    if (hasPreviousPage != d->hasPreviousPage) {
        d->hasPreviousPage = hasPreviousPage;
        emit pagesChanged();
    }

    // request messages
    for (int i = chats.size() - 1; i >= 0; i--) {
        if (QXmppUtils::jidToBareJid(chats[i].with()) == d->jid) {
            HistoryQueueItem item;
            item.with = chats[i].with();
            item.start = chats[i].start();
            d->messageQueue << item;
        }
    }
    d->fetchMessages();
}

