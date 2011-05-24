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

#ifndef __WILINK_CONSOLE_H__
#define __WILINK_CONSOLE_H__

#include "QXmppLogger.h"

#include "chat_model.h"

class LogModel : public ChatModel
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QXmppLogger* logger READ logger WRITE setLogger NOTIFY loggerChanged)

public:
    LogModel(QObject *parent = 0);

    bool enabled() const;
    void setEnabled(bool enabled);

    QXmppLogger* logger() const;
    void setLogger(QXmppLogger *logger);

    // QAbstractItemModel
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
    void enabledChanged(bool enabled);
    void loggerChanged(QXmppLogger *logger);

public slots:
    void clear();

private slots:
    void messageReceived(QXmppLogger::MessageType type, const QString &msg);

private:
    bool m_enabled;
    QXmppLogger *m_logger;
};

#endif
