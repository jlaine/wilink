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

#ifndef QXMPPSHAREEXTENSION_H
#define QXMPPSHAREEXTENSION_H

#include "QXmppClientExtension.h"

class QXmppShareDatabase;
class QXmppShareGetIq;
class QXmppShareSearchIq;
class QXmppShareItem;
class QXmppShareLocation;
class QXmppTransferJob;

class QXmppShareManagerPrivate;

class QXmppShareTransfer : public QXmppLoggable
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum State {
        RequestState = 0,
        TransferState,
        FinishedState,
    };

    qint64 doneBytes() const;
    bool error() const;
    qint64 speed() const;
    State state() const;
    qint64 totalBytes() const;

signals:
    void finished();
    void stateChanged(State state);

public slots:
    void abort();

private slots:
    void _q_getFinished(QXmppShareGetIq &iq);
    void _q_timeout();
    void _q_transferFinished();
    void _q_transferProgress(qint64 done, qint64 total);
    void _q_transferReceived(QXmppTransferJob *job);

private:
    QXmppShareTransfer(QXmppClient *client, const QXmppShareItem &item, QObject *parent);
    void setState(QXmppShareTransfer::State state);

    QString packetId;

    QXmppClient *m_client;
    bool m_error;
    QXmppTransferJob *m_job;
    QString m_sid;
    State m_state;
    QString m_targetDir;

    qint64 m_doneBytes;
    qint64 m_totalBytes;

    friend class QXmppShareManager;
};

class QXmppShareManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    QXmppShareManager(QXmppClient *client, QXmppShareDatabase *db);
    ~QXmppShareManager();

    QXmppShareDatabase *database() const;
    QXmppShareTransfer *get(const QXmppShareItem &item);
    QString search(const QXmppShareLocation &location, int depth, const QString &query);

    /// \cond
    QStringList discoveryFeatures() const;
    bool handleStanza(const QDomElement &element);
    /// \endcond

signals:
    void shareSearchIqReceived(const QXmppShareSearchIq &iq);

private slots:
    void getFinished(const QXmppShareGetIq &iq, const QXmppShareItem &shareItem);
    void searchFinished(const QXmppShareSearchIq &responseIq);
    void _q_transferDestroyed(QObject *obj);

private:
    QXmppShareManagerPrivate * const d;
};

#endif
