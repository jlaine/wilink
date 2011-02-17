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

#ifndef __WILINK_PLAYER_H__
#define __WILINK_PLAYER_H__

#include <QAbstractItemModel>
#include <QUrl>
#include <QTreeView>

#include "chat_panel.h"

class Chat;
class QPushButton;
class QSoundPlayer;

class PlayerModelPrivate;

class PlayerModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PlayerModel(QObject *parent = 0);
    ~PlayerModel();

    bool addUrl(const QUrl &url);
    QModelIndex cursor() const;
    void setCursor(const QModelIndex &index);
    Q_INVOKABLE QModelIndex row(int row);

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex & index) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent) const;

public slots:
    void play(const QModelIndex &index);
    void stop();

signals:
    void cursorChanged(const QModelIndex &index);

private slots:
    void dataReceived();
    void imageReceived();
    void finished(int id);

private:
    PlayerModelPrivate *d;
    friend class PlayerModelPrivate;
};

/** The PlayerPanel class represents a panel for playing media.
 */
class PlayerPanel : public ChatPanel
{
    Q_OBJECT

public:
    PlayerPanel(Chat *chatWindow);

private slots:
    void cursorChanged(const QModelIndex &index);
    void play();
    void rosterDrop(QDropEvent *event, const QModelIndex &index);

private:
    Chat *m_chat;
    PlayerModel *m_model;
    QSoundPlayer *m_player;
    QTreeView *m_view;

    QPushButton *m_playButton;
    QPushButton *m_stopButton;
};

class PlayerView : public QTreeView
{
    Q_OBJECT

public:
    PlayerView(QWidget *parent = 0);
    void setModel(PlayerModel *model);

protected:
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *e);
};

#endif
