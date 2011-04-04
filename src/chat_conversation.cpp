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
#include <QPushButton>
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
};

ChatConversation::ChatConversation(QWidget *parent)
    : ChatPanel(parent),
    m_spacerItem(0)
{
    bool check;
    d = new ChatConversationPrivate;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);

    /* status bar */
    layout->addLayout(headerLayout());

    /* chat history model */
    d->historyModel = new ChatHistoryModel(this);

    /* chat history */
    chatHistory = new QGraphicsView;
    chatHistory->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    chatHistory->setScene(new QGraphicsScene(chatHistory));
    chatHistory->setContextMenuPolicy(Qt::ActionsContextMenu);

    ChatHistoryWidget *chatHistoryWidget = new ChatHistoryWidget;
    chatHistoryWidget->setModel(d->historyModel);
    chatHistory->scene()->addItem(chatHistoryWidget);
    chatHistoryWidget->setView(chatHistory);
    check = connect(chatHistoryWidget, SIGNAL(messageClicked(QModelIndex)),
                    this, SIGNAL(messageClicked(QModelIndex)));
    Q_ASSERT(check);

    panelBar = new ChatPanelBar(chatHistory);
    panelBar->setZValue(10);
    chatHistory->scene()->addItem(panelBar);
    check = connect(chatHistory->scene(), SIGNAL(sceneRectChanged(QRectF)),
                    panelBar, SLOT(reposition()));
    Q_ASSERT(check);

#ifdef USE_DECLARATIVE
    d->imageProvider = new RosterImageProvider;

    QDeclarativeView *view = new QDeclarativeView;
    QDeclarativeContext *ctxt = view->rootContext();
    ctxt->setContextProperty("historyModel", d->historyModel);
    view->engine()->addImageProvider("roster", d->imageProvider);
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    view->setSource(QUrl("qrc:/history.qml"));
    layout->addWidget(view, 1);
#endif

    layout->addWidget(chatHistory, 1);
    filterDrops(chatHistory->viewport());

    /* spacer */
    m_spacerItem = new QSpacerItem(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addSpacerItem(m_spacerItem);

    /* search bar */
    chatSearch = new ChatSearchBar;
    chatSearch->hide();
    check = connect(chatSearch, SIGNAL(displayed(bool)),
                    this, SLOT(slotSearchDisplayed(bool)));
    Q_ASSERT(check);

    check = connect(chatSearch, SIGNAL(find(QString, QTextDocument::FindFlags, bool)),
                    chatHistoryWidget, SLOT(find(QString, QTextDocument::FindFlags, bool)));
    Q_ASSERT(check);

    check = connect(chatSearch, SIGNAL(findClear()),
                    chatHistoryWidget, SLOT(findClear()));
    Q_ASSERT(check);

    check = connect(chatHistoryWidget, SIGNAL(findFinished(bool)),
                    chatSearch, SLOT(findFinished(bool)));
    Q_ASSERT(check);

    layout->addWidget(chatSearch);

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
    connect(this, SIGNAL(findPanel()), chatSearch, SLOT(activate()));
    connect(this, SIGNAL(findAgainPanel()), chatSearch, SLOT(findNext()));
}

ChatConversation::~ChatConversation()
{
    delete d;
}

void ChatConversation::addWidget(ChatPanelWidget *widget)
{
    panelBar->addWidget(widget);
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
}

void ChatConversation::slotSearchDisplayed(bool visible)
{
    QVBoxLayout *vbox = static_cast<QVBoxLayout*>(layout());
    if (visible)
        m_spacerItem->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    else {
        chatHistoryWidget->findClear();
        m_spacerItem->changeSize(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    vbox->invalidate();
}

