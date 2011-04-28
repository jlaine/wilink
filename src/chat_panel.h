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

#include <QGraphicsTextItem>
#include <QGraphicsWidget>
#include <QFrame>
#include <QList>
#include <QWidget>
#include "chat_roster.h"

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

    QAction *addAction(const QIcon &icon, const QString &text);
    virtual void addWidget(QGraphicsWidget *widget);
    virtual ChatRosterModel::Type objectType() const;
    void setWindowIcon(const QIcon &icon);
    void setWindowExtra(const QString &extra);
    void setWindowHelp(const QString &help);
    void setWindowStatus(const QString &status);
    void setWindowTitle(const QString &title);
    static QPalette palette();

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
    void addWidget(QGraphicsWidget *widget);

protected:
    bool eventFilter(QObject *watched, QEvent *Event);
    void updateGeometry();

private slots:
    void trackView();

private:
    QPropertyAnimation *m_animation;
    QTimer *m_delay;
    QGraphicsLinearLayout *m_layout;
    QGraphicsView *m_view;
};

/** The ChatPanelButton class represents a button.
 */
class ChatPanelButton : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatPanelButton(QGraphicsItem *parent);
    void setGeometry(const QRectF &rect);
    void setPixmap(const QPixmap &pixmap);

signals:
    void clicked();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    QGraphicsPathItem *m_path;
    QGraphicsPixmapItem *m_pixmap;
};

/** The ChatPanelImage class represents an image.
 */
class ChatPanelImage : public QGraphicsPixmapItem, public QGraphicsLayoutItem
{
public:
    ChatPanelImage(QGraphicsItem *parent = 0);
    void setGeometry(const QRectF &rect);

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
};

/** The ChatPanelProgress class represents a progress bar.
 */
class ChatPanelProgress : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatPanelProgress(QGraphicsItem *parent);
    void setGeometry(const QRectF &rect);

public slots:
    void setMaximum(int maximum);
    void setMinimum(int maximum);
    void setValue(int value);

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    void resizeBar();

    QGraphicsPathItem *m_bar;
    int m_maximum;
    int m_minimum;
    QRectF m_rect;
    QGraphicsPathItem *m_track;
    int m_value;
};

/** The ChatPanelText class represents a text label.
 */
class ChatPanelText : public QGraphicsTextItem, public QGraphicsLayoutItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsLayoutItem)

public:
    ChatPanelText(const QString &text = QString(), QGraphicsItem *parent = 0);
    void setGeometry(const QRectF &rect);

signals:
    void clicked();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;
};

/** ChatPanelWidget is a wrapper for "task" widgets inside a ChatPanelBar.
 */
class ChatPanelWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    ChatPanelWidget(QGraphicsWidget *contents, QGraphicsItem *parent = 0);
    void setGeometry(const QRectF &rect);

private slots:
    void disappear();

private:
    void appear();
    QGraphicsPathItem *m_border;
    QGraphicsWidget *m_centralWidget;
};

#endif
