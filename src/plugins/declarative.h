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

#ifndef __WILINK_DECLARATIVE_H__
#define __WILINK_DECLARATIVE_H__

#include <QAbstractItemModel>
#include <QFileDialog>
#include <QInputDialog>

#include "QXmppClient.h"
#include "QXmppMessage.h"

class QXmppCallManager;
class QXmppDiscoveryManager;
class QXmppTransferManager;

class QDeclarativeInputDialog : public QInputDialog
{
    Q_OBJECT
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText)
    Q_PROPERTY(QString textValue READ textValue WRITE setTextValue)

public:
    QDeclarativeInputDialog(QWidget *parent = 0) : QInputDialog(parent) {}
};

class QDeclarativeFileDialog : public QFileDialog
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedFiles READ selectedFiles)

public:
    QDeclarativeFileDialog(QWidget *parent = 0) : QFileDialog(parent) {}
};

class QXmppDeclarativeClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString jid READ jid NOTIFY jidChanged)
    Q_PROPERTY(QXmppLogger* logger READ logger CONSTANT)
    Q_PROPERTY(QXmppCallManager* callManager READ callManager CONSTANT)
    Q_PROPERTY(QXmppDiscoveryManager* discoveryManager READ discoveryManager CONSTANT)
    Q_PROPERTY(QXmppRosterManager* rosterManager READ rosterManager CONSTANT)
    Q_PROPERTY(QXmppTransferManager* transferManager READ transferManager CONSTANT)

public:
    QXmppDeclarativeClient(QXmppClient *client);

    QString jid() const;
    QXmppLogger *logger() const;
    QXmppCallManager *callManager() const;
    QXmppDiscoveryManager *discoveryManager() const;
    QXmppRosterManager *rosterManager() const;
    QXmppTransferManager* transferManager() const;

signals:
    void jidChanged(const QString &jid);

private slots:
    void _q_connected();

private:
    QXmppClient *m_client;
};

class QXmppDeclarativeMessage : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State {
        None = QXmppMessage::None,
        Active = QXmppMessage::Active,
        Inactive = QXmppMessage::Inactive,
        Gone = QXmppMessage::Gone,
        Composing = QXmppMessage::Composing,
        Paused = QXmppMessage::Paused,
    };
};

class ListHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QObject* model READ model WRITE setModel NOTIFY modelChanged)

public:
    ListHelper(QObject *parent = 0);

    int count() const;
    Q_INVOKABLE QVariant get(int row) const;

    QObject *model() const;
    void setModel(QObject *model);

signals:
    void countChanged();
    void modelChanged(QObject *model);

private:
    QAbstractItemModel *m_model;
};

#endif
