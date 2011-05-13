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

#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDebug>
#include <QLayout>

#include "chat_conversation.h"
#include "chat_history.h"
#include "chat_roster.h"

class ChatConversationPrivate
{
public:
    ChatHistoryModel *historyModel;
    QDeclarativeView *historyView;
    ChatRosterImageProvider *imageProvider;
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

    /* chat history */
    d->imageProvider = new ChatRosterImageProvider;
    d->historyModel = new ChatHistoryModel(this);
    d->historyView = new QDeclarativeView;
    QDeclarativeContext *ctxt = d->historyView->rootContext();
    ctxt->setContextProperty("historyModel", d->historyModel);
    ctxt->setContextProperty("textHelper", new ChatHistoryHelper(this));
    d->historyView->engine()->addImageProvider("roster", d->imageProvider);
    d->historyView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->historyView->setSource(QUrl("qrc:/conversation.qml"));

    QObject *item = d->historyView->rootObject()->findChild<QObject*>("historyView");
    Q_ASSERT(item);
    check = connect(d->historyModel, SIGNAL(bottomChanged()),
                    item, SLOT(onBottomChanged()));

    layout->addWidget(d->historyView);

#if 0
    /* spacer */
    d->spacerItem = new QSpacerItem(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addSpacerItem(d->spacerItem);

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
    layout->addWidget(d->searchBar);

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

    setLayout(layout);
}

ChatConversation::~ChatConversation()
{
    delete d;
}

QObject *ChatConversation::chatInput()
{
    return d->historyView->rootObject()->findChild<QObject*>("chatInput");
}

ChatHistoryModel *ChatConversation::historyModel()
{
    return d->historyModel;
}

QDeclarativeView *ChatConversation::historyView()
{
    return d->historyView;
}

void ChatConversation::setRosterModel(ChatRosterModel *model)
{
    d->historyModel->setRosterModel(model);
    d->imageProvider->setRosterModel(model);
}

#if 0
// FIXME: restore this
void ChatConversation::slotSearchDisplayed(bool visible)
{
    QVBoxLayout *vbox = static_cast<QVBoxLayout*>(layout());
    if (visible)
        d->spacerItem->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    else {
        //chatHistoryWidget->findClear();
        d->spacerItem->changeSize(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    vbox->invalidate();
}
#endif

