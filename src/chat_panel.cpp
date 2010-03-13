/*
 * wDesktop
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

#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "chat_panel.h"

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent)
{
}

QLayout* ChatPanel::statusBar(const QString &iconName)
{
    /* status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *nameLabel = new QLabel(QString("<b>%1</b>").arg(windowTitle()));
    hbox->addSpacing(16);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QPixmap(iconName));
    hbox->addWidget(iconLabel);
    QPushButton *button = new QPushButton;
    button->setFlat(true);
    button->setMaximumWidth(32);
    button->setIcon(QIcon(":/close.png"));
    connect(button, SIGNAL(clicked()), this, SIGNAL(closeTab()));
    hbox->addWidget(button);
    return hbox;
}

