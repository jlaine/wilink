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

#if defined(USE_LIBNOTIFY)
#include "notifications_libnotify.h"
#elif defined(USE_GROWL)
#include "notifications_growl.h"
#else
#include "notifications.h"
#endif

#include <QCoreApplication>

Notifier::Notifier(QObject *parent)
    : QObject(parent)
{
#if defined(USE_LIBNOTIFY)
    d = new NotifierBackendLibnotify(this);
#elif defined(USE_GROWL)
    d = new NotifierBackendGrowl(this);
#else
    d = 0;
#endif
}

Notifier::~Notifier()
{
    if (d)
        delete d;
}

Notification *Notifier::showMessage(const QString &title, const QString &message, const QString &action)
{
    if (d)
        return d->showMessage(title, message, action);
    else
        return 0;

#if defined(USE_SYSTRAYZ)
    if (d->trayIcon)
    {
        handle = new Notification(this);
        if (d->trayNotification)
            delete d->trayNotification;
        d->trayNotification = handle;
        d->trayIcon->showMessage(title, message);
        return handle;
    }
#endif
}

