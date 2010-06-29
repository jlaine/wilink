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

#include "chat_search.h"

ChatSearchBar::ChatSearchBar(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout;

    QLabel *findIcon = new QLabel;
    findIcon->setPixmap(QPixmap(":/search.png").scaled(16, 16));
    hbox->addWidget(findIcon);

    findBox = new QLineEdit;
    connect(findBox, SIGNAL(returnPressed()), this, SIGNAL(searchForward()));
    hbox->addWidget(findBox);

    QPushButton *prev = new QPushButton(QIcon(":/back.png"), QString());
    connect(prev, SIGNAL(clicked()), this, SIGNAL(searchBackward()));
    hbox->addWidget(prev);

    QPushButton *next = new QPushButton(QIcon(":/forward.png"), QString());
    connect(next, SIGNAL(clicked()), this, SIGNAL(searchForward()));
    hbox->addWidget(next);

    findCase = new QCheckBox(tr("Match case"));
    hbox->addWidget(findCase);
    
    setLayout(hbox);
    setFocusProxy(findBox);
}

bool ChatSearchBar::caseSensitive() const
{
    return (findCase->checkState() == Qt::Checked);
}

QString ChatSearchBar::search() const
{
    return findBox->text();
}

