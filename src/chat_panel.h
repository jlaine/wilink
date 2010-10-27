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

#ifndef __WILINK_CHAT_PANEL_H__
#define __WILINK_CHAT_PANEL_H__

#include <QGraphicsWidget>
#include <QFrame>
#include <QWidget>
#include "chat_roster_item.h"

class QGraphicsLinearLayout;
class QGraphicsView;
class QHBoxLayout;
class QPropertyAnimation;
class QTimer;
class ChatPanelPrivate;
class ChatPanelWidget;

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

    virtual void addWidget(ChatPanelWidget *widget);
    virtual ChatRosterItem::Type objectType() const;
    void setWindowIcon(const QIcon &icon);
    void setWindowExtra(const QString &extra);
    void setWindowHelp(const QString &help);
    void setWindowStatus(const QString &status);
    void setWindowTitle(const QString &title);

protected:
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void filterDrops(QWidget *widget);
    QLayout *headerLayout();
    void queueNotification(const QString &message, int options = 0);

signals:
    void attachPanel();
    void dropPanel(QDropEvent *event);
    void findPanel();
    void findAgainPanel();
    void hidePanel();
    void notifyPanel(const QString &message, int options);
    void registerPanel();
    void showPanel();
    void unregisterPanel();

private slots:
    void sendNotifications();

private:
    ChatPanelPrivate * const d;
};

/** The ChatPanelBar class is used to display a set of "task" widgets.
 */
class ChatPanelBar : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatPanelBar(QGraphicsView *view);
    void addWidget(ChatPanelWidget *widget);

protected:
    bool eventFilter(QObject *watched, QEvent *Event);

private slots:
    void scrollChanged();
    void trackView();

private:
    QPropertyAnimation *m_animation;
    QTimer *m_delay;
    QGraphicsLinearLayout *m_layout;
    QGraphicsView *m_view;
};

/** ChatPanelWidget is the base class for "task" widgets displayed inside
 *  a ChatPanel.
 */
class ChatPanelWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatPanelWidget(QGraphicsItem *parent = 0);
    QRectF contentsRect() const;
    virtual void setGeometry(const QRectF &rect);
    void setButtonEnabled(bool enabled);
    void setButtonPixmap(const QPixmap &pixmap);
    void setButtonToolTip(const QString &toolTip);
    void setIconPixmap(const QPixmap &pixmap);

signals:
    void buttonClicked();
    void contentsClicked();

public slots:
    void appear();
    void disappear();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QGraphicsPathItem *m_border;
    QGraphicsPixmapItem *m_icon;
    bool m_buttonEnabled;
    QGraphicsPathItem *m_buttonPath;
    QGraphicsPixmapItem *m_buttonPixmap;
};

#endif
