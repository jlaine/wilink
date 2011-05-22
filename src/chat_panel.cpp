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

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDropEvent>
#include <QLabel>
#include <QLayout>
#include <QScrollBar>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>

#include "chat_panel.h"

class ChatPanelPrivate
{
public:
    void updateTitle();

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

    // icon and label
    d->iconLabel = new QLabel;
    d->nameLabel = new QLabel;

    // help label
    d->helpLabel = new QLabel(this, Qt::Widget);
    d->helpLabel->setObjectName("panel-help");
    d->helpLabel->setWordWrap(true);
    d->helpLabel->setOpenExternalLinks(true);
    d->helpLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) ;
    d->helpLabel->hide();

    setMinimumWidth(300);
}

ChatPanel::~ChatPanel()
{
    delete d;
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

void ChatPanel::closeEvent(QCloseEvent *event)
{
    emit hidePanel();
    QWidget::closeEvent(event);
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

/** Create a custom palette for chat panel */
QPalette ChatPanel::palette()
{
    QPalette palette;

    // colors for active and inactive content
    palette.setColor(QPalette::Light, QColor("#E7F4FE"));
    palette.setColor(QPalette::Midlight, QColor("#BFDDF4"));
    palette.setColor(QPalette::Button, QColor("#88BFE9"));
    palette.setColor(QPalette::Mid, QColor("#4A9DDD"));
    palette.setColor(QPalette::Dark, QColor("#2689D6"));
    palette.setColor(QPalette::Shadow, QColor("#aaaaaa"));
    palette.setColor(QPalette::ButtonText, QColor("#000000"));
    palette.setColor(QPalette::BrightText, QColor("#FFFFFF"));

    // colors for disabled content
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor("#F5FAFE"));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor("#E5F1FA"));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor("#CFE5F6"));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor("#B6D7F1"));
    palette.setColor(QPalette::Disabled, QPalette::Dark, QColor("#A8CFEE"));
    palette.setColor(QPalette::Disabled, QPalette::Shadow, QColor("#D6D6D6"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#999999"));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor("#FFFFFF"));

    return palette;
}

