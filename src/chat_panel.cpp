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

class ChatPanelPrivate
{
public:
    void updateTitle();

    QVBoxLayout *header;
    QHBoxLayout *hbox;
    QHBoxLayout *widgets;
    QPushButton *attachButton;
    QPushButton *closeButton;
    QLabel *helpLabel;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QString windowExtra;
    QString windowStatus;
    QList< QPair<QString, int> > notificationQueue;

    ChatPanel *q;
};

void ChatPanelPrivate::updateTitle()
{
    nameLabel->setText(QString("<b>%1</b> %2<br/>%3").arg(q->windowTitle(),
        windowStatus, windowExtra));
}

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent),
    d(new ChatPanelPrivate)
{
    bool check;
    d->q = this;

    d->attachButton = new QPushButton;
    d->attachButton->setFlat(true);
    d->attachButton->setMaximumWidth(32);
    d->attachButton->setIcon(QIcon(":/add.png"));
    d->attachButton->hide();
    check = connect(d->attachButton, SIGNAL(clicked()),
                    this, SIGNAL(attachPanel()));
    Q_ASSERT(check);

    d->closeButton = new QPushButton;
    d->closeButton->setFlat(true);
    d->closeButton->setMaximumWidth(32);
    d->closeButton->setIcon(QIcon(":/close.png"));
    check = connect(d->closeButton, SIGNAL(clicked()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    d->iconLabel = new QLabel;
    d->nameLabel = new QLabel;

    d->hbox = new QHBoxLayout;
    d->hbox->addSpacing(16);
    d->hbox->addWidget(d->nameLabel);
    d->hbox->addStretch();
    d->hbox->addWidget(d->iconLabel);
    d->hbox->addWidget(d->attachButton);
    d->hbox->addWidget(d->closeButton);

    // assemble header
    d->header = new QVBoxLayout;
    d->header->setMargin(0);
    d->header->setSpacing(10);
    d->header->addLayout(d->hbox);

    d->helpLabel = new QLabel;
    d->helpLabel->setWordWrap(true);
    d->helpLabel->setOpenExternalLinks(true);
    d->helpLabel->hide();
    d->header->addWidget(d->helpLabel);

    d->widgets = new QHBoxLayout;
    d->widgets->addStretch();
    d->header->addLayout(d->widgets);

    setMinimumWidth(300);
    setStyleSheet("ChatPanelWidget { \
        background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
            stop: 0 #E0E0E0, \
            stop: 0.6 #FFFFFF, \
            stop: 1 #E0E0E0); \
        border: 1px solid gray; \
        border-radius: 5px; }");
}

ChatPanel::~ChatPanel()
{
    delete d;
}

void ChatPanel::addWidget(ChatPanelWidget *widget)
{
    d->widgets->insertWidget(d->widgets->count() - 1, widget);
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
    d->windowExtra = extra;
    d->updateTitle();
}

/** Sets the window's help text.
 *
 * @param help
 */
void ChatPanel::setWindowHelp(const QString &help)
{
    d->helpLabel->setText(help);
#ifndef WILINK_EMBEDDED
    if (help.isEmpty())
        d->helpLabel->hide();
    else
        d->helpLabel->show();
#endif
}

/** When the window icon is set, update the header icon.
 *
 * @param icon
 */
void ChatPanel::setWindowIcon(const QIcon &icon)
{
    QWidget::setWindowIcon(icon);
    const QSize actualSize = icon.actualSize(QSize(64, 64));
    d->iconLabel->setPixmap(icon.pixmap(actualSize));
}

/** When additional text is set, update the header text.
 */
void ChatPanel::setWindowStatus(const QString &status)
{
    d->windowStatus = status;
    d->updateTitle();
}

/** When the window title is set, update the header text.
 *
 * @param title
 */
void ChatPanel::setWindowTitle(const QString &title)
{
    QWidget::setWindowTitle(title);
    d->updateTitle();
}

/** Return a layout object for the panel header.
 */
QLayout* ChatPanel::headerLayout()
{
    return d->header;
}

void ChatPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ParentChange)
    {
        if (parent())
        {
            layout()->setMargin(0);
            d->attachButton->hide();
            d->closeButton->show();
        } else {
            layout()->setMargin(6);
            d->attachButton->show();
            d->closeButton->hide();
        }
    }
    QWidget::changeEvent(event);
}

void ChatPanel::closeEvent(QCloseEvent *event)
{
    emit hidePanel();
    QWidget::closeEvent(event);
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
    d->notificationQueue << qMakePair(message, options);
    QTimer::singleShot(0, this, SLOT(sendNotifications()));
}

void ChatPanel::sendNotifications()
{
    while (!d->notificationQueue.isEmpty())
    {
        QPair<QString, int> entry = d->notificationQueue.takeFirst();
        emit notifyPanel(entry.first, entry.second);
    }
}

ChatPanelWidget::ChatPanelWidget(QWidget *parent)
    : QFrame(parent)
{
}
