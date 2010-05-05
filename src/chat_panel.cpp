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

#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "chat_panel.h"

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent)
{
    closeButton = new QPushButton;
    closeButton->setFlat(true);
    closeButton->setMaximumWidth(32);
    closeButton->setIcon(QIcon(":/close.png"));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(hidePanel()));

    iconLabel = new QLabel;
    nameLabel = new QLabel;
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowExtra(const QString &extra)
{
    windowExtra = extra;
    nameLabel->setText(QString("<b>%1</b> %2").arg(windowTitle(), windowExtra));
}

/** When the window icon is set, update the header icon.
 *
 * @param icon
 */
void ChatPanel::setWindowIcon(const QIcon &icon)
{
    QWidget::setWindowIcon(icon);
    const QSize actualSize = icon.actualSize(QSize(80, 80));
    iconLabel->setPixmap(icon.pixmap(actualSize));
}

/** When the window title is set, update the header text.
 *
 * @param title
 */
void ChatPanel::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
    nameLabel->setText(QString("<b>%1</b> %2").arg(windowTitle(), windowExtra));
}

/** Return a layout object for the panel header.
 */
QLayout* ChatPanel::headerLayout()
{
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addSpacing(16);
    hbox->addWidget(nameLabel);
    hbox->addStretch();
    hbox->addWidget(iconLabel);
    hbox->addWidget(closeButton);
    return hbox;
}

