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

#ifndef __WILINK_CHAT_PANEL_H__
#define __WILINK_CHAT_PANEL_H__

#include <QList>
#include <QWidget>

class QHBoxLayout;
class QTimer;
class ChatPanelPrivate;

/** ChatPanel is the base class for all the panels displayed in the right-hand
 *  part of the user interface, such as conversations.
 */
class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    enum NotificationOptions
    {
        ForceNotification = 1,
    };

public:
    ChatPanel(QWidget *parent);
    ~ChatPanel();

    void setWindowIcon(const QIcon &icon);
    void setWindowExtra(const QString &extra);
    void setWindowHelp(const QString &help);
    void setWindowStatus(const QString &status);
    void setWindowTitle(const QString &title);
    static QPalette palette();

protected:
    void closeEvent(QCloseEvent *event);
    void queueNotification(const QString &message, int options = 0);

signals:
    void attachPanel();
    void findPanel();
    void findAgainPanel();
    void hidePanel();
    void notifyPanel(const QString &message, int options);
    void showPanel();

private slots:
    void sendNotifications();

private:
    ChatPanelPrivate * const d;
};

#endif
