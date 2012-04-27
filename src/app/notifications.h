/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __NOTIFIER_H__
#define __NOTIFIER_H__

#include <QObject>

class NotifierPrivate;

class Notification : public QObject
{
    Q_OBJECT

public:
    Notification(QObject *parent = 0) : QObject(parent)
    {}

signals:
    void clicked();
};

class Notifier : public QObject
{
    Q_OBJECT

public:
    Notifier(QObject *parent = 0);
    ~Notifier();

public slots:
    Notification *showMessage(const QString &title, const QString &message, const QString &action);

private:
    NotifierPrivate *d;
};

#endif
