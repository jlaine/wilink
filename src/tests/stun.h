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

#include <QObject>

#include "QXmppLogger.h"

class QHostAddress;
class QXmppTurnAllocation;

class TurnTester : public QObject
{
    Q_OBJECT

public:
    TurnTester(QXmppTurnAllocation *allocation);

signals:
    void finished();

private slots:
    void connected();
    void datagramReceived(const QByteArray &data, const QHostAddress &host, quint16 port);

private:
    QXmppTurnAllocation *m_allocation;
};

