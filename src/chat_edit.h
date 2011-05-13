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

#ifndef __WILINK_CHAT_EDIT_H__
#define __WILINK_CHAT_EDIT_H__

#include <QTextEdit>

#include "QXmppMessage.h"

class QKeyEvent;

class ChatEditPrivate;

class ChatEdit : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int chatState READ chatState NOTIFY chatStateChanged);
    Q_PROPERTY(QStringList participants READ participants WRITE setParticipants)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    ChatEdit(int maxheight = 80, QWidget* parent = NULL);
    ~ChatEdit();

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;

    int chatState() const;

    QStringList participants() const;
    void setParticipants(const QStringList &participants);

    QString text() const;
    void setText(const QString &text);

signals:
    void chatStateChanged(int state);
    void focused();
    void returnPressed();
    void textChanged();

protected:
    virtual void focusInEvent(QFocusEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void resizeEvent(QResizeEvent *e);

public slots:
    void onTextChanged();
    void talkAt(const QString &text);

private slots:
    void slotInactive();
    void slotPaused();

private:
    ChatEditPrivate * const d;
};
#endif
