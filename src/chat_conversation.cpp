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
#include <QDebug>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>

#include "chat_conversation.h"
#include "chat_edit.h"
#include "chat_history.h"
#include "chat_search.h"

#ifdef Q_OS_MAC
#define SPACING 6
#else
#define SPACING 2
#endif

#ifdef USE_DECLARATIVE
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeImageProvider>
#include <QDeclarativeView>

class RosterImageProvider : public QDeclarativeImageProvider
{
public:
    RosterImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
    void setRosterModel(ChatRosterModel *rosterModel);

private:
    ChatRosterModel *m_rosterModel;
};

RosterImageProvider::RosterImageProvider()
    : QDeclarativeImageProvider(Pixmap),
    m_rosterModel(0)
{
}

QPixmap RosterImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_ASSERT(m_rosterModel);
    const QPixmap pixmap = m_rosterModel->contactAvatar(id);
    if (size)
        *size = pixmap.size();
    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);
    else
        return pixmap;
}

void RosterImageProvider::setRosterModel(ChatRosterModel *rosterModel)
{
    m_rosterModel = rosterModel;
}

#endif

class ChatConversationPrivate
{
public:
    ChatHistoryModel *historyModel;
#ifdef USE_DECLARATIVE
    RosterImageProvider *imageProvider;
#endif
    ChatPanelBar *panelBar;
    ChatSearchBar *searchBar;
    QSpacerItem *spacerItem;
};

ChatConversation::ChatConversation(QWidget *parent)
    : ChatPanel(parent)
{
    bool check;
    d = new ChatConversationPrivate;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);

    /* status bar */
    layout->addLayout(headerLayout());

    // chatHistory and chatRoomList separation
    QSplitter *splitter = new QSplitter;
    layout->addWidget(splitter);

    /* search bar */
    d->searchBar = new ChatSearchBar;
    d->searchBar->hide();
    check = connect(d->searchBar, SIGNAL(displayed(bool)),
                    this, SLOT(slotSearchDisplayed(bool)));
    Q_ASSERT(check);
    check = connect(this, SIGNAL(findPanel()),
                    d->searchBar, SLOT(activate()));
    Q_ASSERT(check);
    check = connect(this, SIGNAL(findAgainPanel()),
                    d->searchBar, SLOT(findNext()));
    Q_ASSERT(check);

    /* chat history model */
    d->historyModel = new ChatHistoryModel(this);

    /* chat history */
#ifdef USE_DECLARATIVE
    d->imageProvider = new RosterImageProvider;

    QDeclarativeView *view = new QDeclarativeView;
    QDeclarativeContext *ctxt = view->rootContext();
    ctxt->setContextProperty("historyModel", d->historyModel);
    ctxt->setContextProperty("textHelper", new ChatHistoryHelper(this));
    view->engine()->addImageProvider("roster", d->imageProvider);
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/conversation.qml"));
    splitter->addWidget(view);
#else
    QGraphicsView *view = new QGraphicsView;
    view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    view->setScene(new QGraphicsScene(view));
    view->setContextMenuPolicy(Qt::ActionsContextMenu);

    ChatHistoryWidget *chatHistoryWidget = new ChatHistoryWidget;
    chatHistoryWidget->setModel(d->historyModel);
    view->scene()->addItem(chatHistoryWidget);
    chatHistoryWidget->setView(view);
    splitter->addWidget(view);

    check = connect(chatHistoryWidget, SIGNAL(messageClicked(QModelIndex)),
                    this, SIGNAL(messageClicked(QModelIndex)));
    Q_ASSERT(check);

    check = connect(d->searchBar, SIGNAL(find(QString, QTextDocument::FindFlags, bool)),
                    chatHistoryWidget, SLOT(find(QString, QTextDocument::FindFlags, bool)));
    Q_ASSERT(check);

    check = connect(d->searchBar, SIGNAL(findClear()),
                    chatHistoryWidget, SLOT(findClear()));
    Q_ASSERT(check);

    check = connect(chatHistoryWidget, SIGNAL(findFinished(bool)),
                    d->searchBar, SLOT(findFinished(bool)));
    Q_ASSERT(check);
#endif

    // chatroom list
    chatRoomList = new QListView();
    chatRoomList->setObjectName("participant-list");
    chatRoomList->setViewMode(QListView::IconMode);
    chatRoomList->setMovement(QListView::Static);
    chatRoomList->setResizeMode(QListView::Adjust);
    chatRoomList->setFlow(QListView::LeftToRight);
    chatRoomList->setGridSize(QSize(64,55));
    chatRoomList->setWordWrap(false);
    chatRoomList->setWrapping(true);
    chatRoomList->hide();
    splitter->addWidget(chatRoomList);

    // set ratio between chat history and participants list
    QList<int> sizes = QList<int>();
    sizes.append(250);
    sizes.append(100);
    splitter->setSizes(sizes);

    d->panelBar = new ChatPanelBar(view);
    d->panelBar->setZValue(10);
#ifndef USE_DECLARATIVE
    view->scene()->addItem(d->panelBar);
#endif
    filterDrops(view->viewport());

    /* spacer */
    d->spacerItem = new QSpacerItem(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addSpacerItem(d->spacerItem);

    /* search bar */
    layout->addWidget(d->searchBar);

    /* text edit */
    chatInput = new ChatEdit(80);
#ifdef WILINK_EMBEDDED
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(chatInput);
    QPushButton *sendButton = new QPushButton;
    sendButton->setFlat(true);
    sendButton->setMaximumWidth(32);
    sendButton->setIcon(QIcon(":/upload.png"));
    check = connect(sendButton, SIGNAL(clicked()),
                    chatInput, SIGNAL(returnPressed()));
    Q_ASSERT(check);
    hbox->addWidget(sendButton);
    layout->addLayout(hbox);
#else
    layout->addWidget(chatInput);
#endif

    setFocusProxy(chatInput);
    setLayout(layout);

    /* shortcuts */
}

ChatConversation::~ChatConversation()
{
    delete d;
}

void ChatConversation::addWidget(QGraphicsWidget *widget)
{
    d->panelBar->addWidget(widget);
}

ChatHistoryModel *ChatConversation::historyModel()
{
    return d->historyModel;
}

void ChatConversation::setRosterModel(ChatRosterModel *model)
{
    d->historyModel->setRosterModel(model);
#ifdef USE_DECLARATIVE
    d->imageProvider->setRosterModel(model);
#endif
    chatRoomList->setModel(model);
}

void ChatConversation::slotSearchDisplayed(bool visible)
{
    QVBoxLayout *vbox = static_cast<QVBoxLayout*>(layout());
    if (visible)
        d->spacerItem->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    else {
        // FIXME: restore this
        //chatHistoryWidget->findClear();
        d->spacerItem->changeSize(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    vbox->invalidate();
}

