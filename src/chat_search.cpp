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
#include <QTimer>
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
    findTimer = new QTimer(this);
    findTimer->setSingleShot(true);
    findTimer->setInterval(100);
    connect(findTimer, SIGNAL(timeout()), this, SLOT(slotSearchDelayed()));

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(0);
    hbox->setSpacing(SPACING);
    hbox->addSpacing(MARGIN);

    QLabel *findIcon = new QLabel;
    findIcon->setPixmap(QPixmap(":/search.png").scaled(16, 16));
    hbox->addWidget(findIcon);

    findPrompt = new QLabel;
    findPrompt->hide();
    hbox->addWidget(findPrompt);

    findBox = new QLineEdit;
    normalPalette = findBox->palette();
    failedPalette = findBox->palette();
    failedPalette.setColor(QPalette::Active, QPalette::Base, QColor(255, 0, 0, 127));
    connect(findBox, SIGNAL(returnPressed()), this, SLOT(findNext()));
    connect(findBox, SIGNAL(textChanged(QString)), this, SLOT(slotSearchChanged()));
    hbox->addWidget(findBox, 1);

    //hbox->addSpacing(BUTTON_MARGIN);

    m_findPrevious = new QPushButton;
    m_findPrevious->setIcon(QIcon(":/back.png"));
    m_findPrevious->setMaximumHeight(BUTTON_SIZE);
    m_findPrevious->setMaximumWidth(BUTTON_SIZE);
    connect(m_findPrevious, SIGNAL(clicked()), this, SLOT(findPrevious()));
    hbox->addWidget(m_findPrevious);

    //hbox->addSpacing(2 * BUTTON_MARGIN);

    m_findNext = new QPushButton;
    m_findNext->setIcon(QIcon(":/forward.png"));
    m_findNext->setMaximumHeight(BUTTON_SIZE);
    m_findNext->setMaximumWidth(BUTTON_SIZE);
    connect(m_findNext, SIGNAL(clicked()), this, SLOT(findNext()));
    hbox->addWidget(m_findNext);

    //hbox->addSpacing(BUTTON_MARGIN);

    m_findCase = new QCheckBox(tr("Match case"));
    connect(m_findCase, SIGNAL(stateChanged(int)), this, SLOT(slotSearchChanged()));
    hbox->addWidget(m_findCase);

    hbox->addStretch();

    m_findClose = new QPushButton;
    m_findClose->setFlat(true);
    m_findClose->setMaximumHeight(BUTTON_SIZE);
    m_findClose->setMaximumWidth(BUTTON_SIZE);
    m_findClose->setIcon(QIcon(":/close.png"));
    connect(m_findClose, SIGNAL(clicked()), this, SLOT(deactivate()));
    hbox->addWidget(m_findClose);

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
    if (m_findCase->checkState() == Qt::Checked)
        flags |= QTextDocument::FindCaseSensitively;
    emit find(findBox->text(), flags, false);
}

void ChatSearchBar::findPrevious()
{
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (m_findCase->checkState() == Qt::Checked)
        flags |= QTextDocument::FindCaseSensitively;
    emit find(findBox->text(), flags, false);
}

void ChatSearchBar::setControlsVisible(bool visible)
{
    if (visible)
    {
        m_findCase->show();
        m_findNext->show();
        m_findPrevious->show();
        m_findClose->show();
    } else {
        m_findCase->hide();
        m_findNext->hide();
        m_findPrevious->hide();
        m_findClose->hide();
    }
}

void ChatSearchBar::setDelay(int msecs)
{
    findTimer->setInterval(msecs);
}

void ChatSearchBar::setText(const QString &text)
{
    findPrompt->setText(text);
    findPrompt->show();
}

void ChatSearchBar::slotSearchChanged()
{
    findBox->setPalette(normalPalette);
    findTimer->start();
}

void ChatSearchBar::slotSearchDelayed()
{
    QTextDocument::FindFlags flags;
    if (m_findCase->checkState() == Qt::Checked)
        flags |= QTextDocument::FindCaseSensitively;
    emit find(findBox->text(), flags, true);
}

