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

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVariant>

#include "chat_search.h"

#define BUTTON_SIZE 24
#ifdef Q_OS_MAC
#define BUTTON_MARGIN (32 - BUTTON_SIZE)/2
#define SPACING 16
#else
#define BUTTON_MARGIN 0
#define SPACING 8
#endif
#define MARGIN 5

ChatSearchBar::ChatSearchBar(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(0);
    hbox->setSpacing(0);

    hbox->addSpacing(MARGIN);

    QLabel *findIcon = new QLabel;
    findIcon->setPixmap(QPixmap(":/search.png").scaled(16, 16));
    hbox->addWidget(findIcon);

    hbox->addSpacing(SPACING);

    findBox = new QLineEdit;
    normalPalette = findBox->palette();
    failedPalette = findBox->palette();
    failedPalette.setColor(QPalette::Active, QPalette::Base, QColor(255, 0, 0, 127));
    connect(findBox, SIGNAL(returnPressed()), this, SLOT(findNext()));
    connect(findBox, SIGNAL(textChanged(QString)), this, SLOT(slotSearchChanged()));
    hbox->addWidget(findBox);

    hbox->addSpacing(SPACING + BUTTON_MARGIN);

    QPushButton *prev = new QPushButton;
    prev->setIcon(QIcon(":/back.png"));
    prev->setMaximumHeight(BUTTON_SIZE);
    prev->setMaximumWidth(BUTTON_SIZE);
    connect(prev, SIGNAL(clicked()), this, SLOT(findPrevious()));
    hbox->addWidget(prev);

    hbox->addSpacing(SPACING + 2 * BUTTON_MARGIN);

    QPushButton *next = new QPushButton;
    next->setIcon(QIcon(":/forward.png"));
    next->setMaximumHeight(BUTTON_SIZE);
    next->setMaximumWidth(BUTTON_SIZE);
    connect(next, SIGNAL(clicked()), this, SLOT(findNext()));
    hbox->addWidget(next);

    hbox->addSpacing(SPACING + BUTTON_MARGIN);

    findCase = new QCheckBox(tr("Match case"));
    connect(findCase, SIGNAL(stateChanged(int)), this, SLOT(slotSearchChanged()));
    hbox->addWidget(findCase);

    hbox->addStretch();

    QPushButton *close = new QPushButton;
    close->setFlat(true);
    close->setMaximumHeight(BUTTON_SIZE);
    close->setMaximumWidth(BUTTON_SIZE);
    close->setIcon(QIcon(":/close.png"));
    connect(close, SIGNAL(clicked()), this, SLOT(deactivate()));
    hbox->addWidget(close);

    hbox->addSpacing(MARGIN);

    setLayout(hbox);
    setFocusProxy(findBox);
}

void ChatSearchBar::activate()
{
    if (!isVisible())
    {
        show();
        emit displayed(true);
    }
    findBox->setFocus();
}

void ChatSearchBar::deactivate()
{
    if (isVisible())
    {
        hide();
        emit displayed(false);
    }
}

void ChatSearchBar::findFinished(bool found)
{
    if (found)
        findBox->setPalette(normalPalette);
    else
        findBox->setPalette(failedPalette);
}

void ChatSearchBar::findNext()
{
    QTextDocument::FindFlags flags;
    if (findCase->checkState() == Qt::Checked)
        flags |= QTextDocument::FindCaseSensitively;
    emit find(findBox->text(), flags);
}

void ChatSearchBar::findPrevious()
{
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (findCase->checkState() == Qt::Checked)
        flags |= QTextDocument::FindCaseSensitively;
    emit find(findBox->text(), flags);
}

void ChatSearchBar::slotSearchChanged()
{
    findBox->setPalette(normalPalette);
}

