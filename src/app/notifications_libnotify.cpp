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

#include "notifications_libnotify.h"

#include <libnotify/notify.h>
#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif

class NotifierBackendLibnotify : public NotifierPrivate
{
public:
    NotifierBackendLibnotify(Notifier *qq);
    Notification *showMessage(const QString &title, const QString &message, const QString &action);

private:
    bool libnotify_accepts_actions;
    Notifier *q;
};

NotifierBackendLibnotify::NotifierBackendLibnotify(Notifier *qq)
    : libnotify_accepts_actions(false)
    , q(qq)
{
    /* initialise libnotify */
    notify_init(qApp->applicationName().toUtf8().constData());

    // test if callbacks are supported
    GList *capabilities = notify_get_server_caps();
    if(capabilities != NULL) {
        for(GList *c = capabilities; c != NULL; c = c->next) {
            if(g_strcmp0((char*)c->data, "actions") == 0 ) {
                libnotify_accepts_actions = true;
                break;
            }
        }
        g_list_foreach(capabilities, (GFunc)g_free, NULL);
        g_list_free(capabilities);
    }
};

static void notificationClicked(NotifyNotification *notification, char *action, gpointer data)
{
    if(g_strcmp0(action, "show-conversation") == 0)
    {
        Notification *handle = static_cast<Notification*>(data);
        QMetaObject::invokeMethod(handle, "clicked");
        notify_notification_close(notification, NULL);
    }
}

static void notificationClosed(NotifyNotification *notification)
{
    if(notification)
    {
        g_object_unref(G_OBJECT(notification));
    }
}

Notification *NotifierBackendLibnotify::showMessage(const QString &title, const QString &message, const QString &action)
{
    Notification *handle = new Notification(q);

    NotifyNotification *notification = notify_notification_new((const char *)title.toUtf8(),
                                                               (const char *)message.toUtf8(),
#if !NOTIFY_CHECK_VERSION(0, 7, 0)
                                                               NULL,
#endif
                                                               NULL);

    if( !notification ) {
        qWarning("Failed to create notification");
        return handle;
    }

    // Set timeout
    notify_notification_set_timeout(notification, NOTIFY_EXPIRES_DEFAULT);

    // set action handled when notification is closed
    g_signal_connect(notification, "closed", G_CALLBACK(notificationClosed), NULL);

    // Set callback if supported
    if(libnotify_accepts_actions) {
        notify_notification_add_action(notification,
                                       "show-conversation",
                                       action.toUtf8(),
                                       (NotifyActionCallback) &notificationClicked,
                                       handle,
                                       FALSE);
    }

    // Schedule notification for showing
    if (!notify_notification_show(notification, NULL))
        qDebug("Failed to send notification");

    return handle;
}

