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

#include "QXmppUtils.h"

#include "history.h"
#include "roster.h"

typedef QPair<QRegExp, QString> TextTransform;
static QList<TextTransform> textTransforms;

static const QRegExp meRegex = QRegExp("^/me[ \t]+(.*)");

class HistoryItem : public ChatModelItem
{
public:
    ~HistoryItem();

    QList<HistoryMessage*> messages;
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
    smileys.insert(":@", ":/smiley-angry.png");
    smileys.insert(":-@", ":/smiley-angry.png");
    smileys.insert(":s", ":/smiley-confused.png");
    smileys.insert(":-s", ":/smiley-confused.png");
    smileys.insert(":)", ":/smiley-happy.png");
    smileys.insert(":-)", ":/smiley-happy.png");
    smileys.insert(":|", ":/smiley-neutral.png");
    smileys.insert(":-|", ":/smiley-neutral.png");
    smileys.insert(":p", ":/smiley-raspberry.png");
    smileys.insert(":-p", ":/smiley-raspberry.png");
    smileys.insert(":(", ":/smiley-sad.png");
    smileys.insert(":-(", ":/smiley-sad.png");
    smileys.insert(";)", ":/smiley-wink.png");
    smileys.insert(";-)", ":/smiley-wink.png");

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
    QMap<QString, VCard*> rosterCards;
};

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
    setRoleNames(roleNames);
}

/** Adds a message in the chat history.
 *
 * The message is checked for duplicates, inserted in chronological order
 * and grouped with related messages.
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

            // check for collision
            if (message.jid == curMessage->jid &&
                message.body == curMessage->body &&
                qAbs(message.date.secsTo(curMessage->date)) < 10)
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

void HistoryModel::cardChanged()
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

/** Clears all messages.
 */
void HistoryModel::clear()
{
    int rows = rowCount(QModelIndex());
    if (rows > 0)
        removeRows(0, rows);
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
                    this, SLOT(cardChanged()));
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
    }

    return QVariant();
}

