/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

ChatConversation::ChatConversation(QWidget *parent)
    : ChatPanel(parent),
    spacerItem(0)
{
    bool check;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);

    /* status bar */
    layout->addLayout(headerLayout());

    /* chat history */
    chatHistory = new ChatHistory;
    layout->addWidget(chatHistory);
    filterDrops(chatHistory->viewport());

    /* spacer */
    spacerItem = new QSpacerItem(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addSpacerItem(spacerItem);

    /* search bar */
    chatSearch = new ChatSearchBar;
    chatSearch->hide();
    check = connect(chatSearch, SIGNAL(displayed(bool)),
                    this, SLOT(slotSearchDisplayed(bool)));
    Q_ASSERT(check);

    check = connect(chatSearch, SIGNAL(find(QString, QTextDocument::FindFlags, bool)),
                    chatHistory->historyWidget(), SLOT(find(QString, QTextDocument::FindFlags, bool)));
    Q_ASSERT(check);

    check = connect(chatSearch, SIGNAL(findClear()),
                    chatHistory->historyWidget(), SLOT(findClear()));
    Q_ASSERT(check);

    check = connect(chatHistory->historyWidget(), SIGNAL(findFinished(bool)),
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

ChatHistoryWidget *ChatConversation::historyWidget()
{
    return chatHistory->historyWidget();
}

void ChatConversation::slotSearchDisplayed(bool visible)
{
    QVBoxLayout *vbox = static_cast<QVBoxLayout*>(layout());
    if (visible)
        spacerItem->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    else {
        chatHistory->historyWidget()->findClear();
        spacerItem->changeSize(16, SPACING, QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    vbox->invalidate();
}

