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

#include <QDropEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTimer>

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

/** Return the type of entry to add to the roster.
 */
ChatRosterItem::Type ChatPanel::objectType() const
{
    return ChatRosterItem::Other;
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowExtra(const QString &extra)
{
    windowExtra = extra;
    updateTitle();
}

/** When the window icon is set, update the header icon.
 *
 * @param icon
 */
void ChatPanel::setWindowIcon(const QIcon &icon)
{
    QWidget::setWindowIcon(icon);
    const QSize actualSize = icon.actualSize(QSize(64, 64));
    iconLabel->setPixmap(icon.pixmap(actualSize));
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowStatus(const QString &status)
{
    windowStatus = status;
    updateTitle();
}

/** When the window title is set, update the header text.
 *
 * @param title
 */
void ChatPanel::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
    updateTitle();
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

bool ChatPanel::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::DragEnter)
    {
        QDragEnterEvent *event = static_cast<QDragEnterEvent*>(e);
        event->acceptProposedAction();
        return true;
    }
    else if (e->type() == QEvent::DragLeave)
    {
        return true;
    }
    else if (e->type() == QEvent::DragMove || e->type() == QEvent::Drop)
    {
        QDropEvent *event = static_cast<QDropEvent*>(e);
        event->ignore();
        emit dropPanel(event);
        return true;
    }
    return false;
}

void ChatPanel::filterDrops(QWidget *widget)
{
    widget->setAcceptDrops(true);
    widget->installEventFilter(this);
}

void ChatPanel::queueNotification(const QString &message, int options)
{
    notificationQueue << qMakePair(message, options);
    QTimer::singleShot(0, this, SLOT(sendNotifications()));
}

void ChatPanel::sendNotifications()
{
    while (!notificationQueue.isEmpty())
    {
        QPair<QString, int> entry = notificationQueue.takeFirst();
        emit notifyPanel(entry.first, entry.second);
    }
}

void ChatPanel::updateTitle()
{
    nameLabel->setText(QString("<b>%1</b> %2<br/>%3").arg(windowTitle(), windowStatus, windowExtra));
}
