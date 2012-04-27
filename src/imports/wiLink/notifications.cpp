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
#include <QMenu>
#include <QSystemTrayIcon>

#include "declarative.h"

QString QDeclarativeFileDialog::directory() const
{
    return QFileDialog::directory().path();
}

void QDeclarativeFileDialog::setDirectory(const QString &directory)
{
    QFileDialog::setDirectory(QDir(directory));
}


class NotifierPrivate
{
public:
    NotifierPrivate();

    NotifierBackend *backend;
#ifdef USE_SYSTRAY
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    Notification *trayNotification;
#endif
};

NotifierPrivate::NotifierPrivate()
    : backend(0)
#ifdef USE_SYSTRAY
    , trayIcon(0)
    , trayMenu(0)
    , trayNotification(0)
#endif
{
}

Notifier::Notifier(QObject *parent)
    : QObject(parent)
{
    d = new NotifierPrivate;
#if defined(USE_LIBNOTIFY)
    d->backend = new NotifierBackendLibnotify(this);
#elif defined(USE_GROWL)
    d->backend = new NotifierBackendGrowl(this);
#endif

#ifdef USE_SYSTRAY
    d->trayIcon = new QSystemTrayIcon;
#ifdef Q_OS_MAC
    d->trayIcon->setIcon(QIcon(":/32x32/wiLink-black.png"));
#else
    d->trayIcon->setIcon(QIcon(":/32x32/wiLink.png"));
#endif

    d->trayMenu = new QMenu;
    QAction *action = d->trayMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered()),
            qApp, SLOT(quit()));
    d->trayIcon->setContextMenu(d->trayMenu);

    connect(d->trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(_q_trayActivated(QSystemTrayIcon::ActivationReason)));
    connect(d->trayIcon, SIGNAL(messageClicked()),
            this, SLOT(_q_trayClicked()));
    d->trayIcon->show();
#endif
}

Notifier::~Notifier()
{
#ifdef USE_SYSTRAY
    // destroy tray icon
    if (d->trayIcon)
    {
#ifdef Q_OS_WIN
        // FIXME : on Windows, deleting the icon crashes the program
        d->trayIcon->hide();
#else
        delete d->trayIcon;
        delete d->trayMenu;
#endif
    }
#endif

    if (d->backend)
        delete d->backend;
    delete d;
}

QFileDialog *Notifier::fileDialog()
{
    QFileDialog *dialog = new QDeclarativeFileDialog(0);
    return dialog;
}

Notification *Notifier::showMessage(const QString &title, const QString &message, const QString &action)
{
    if (d->backend) {
        return d->backend->showMessage(title, message, action);
#if defined(USE_SYSTRAY)
    } else if (d->trayIcon) {
        handle = new Notification(this);
        if (d->trayNotification)
            delete d->trayNotification;
        d->trayNotification = handle;
        d->trayIcon->showMessage(title, message);
        return handle;
#endif
    } else {
        return 0;
    }
}

#ifdef USE_SYSTRAY
void Notifier::_q_trayActivated(QSystemTrayIcon::ActivationReason reason)
{
//    if (reason != QSystemTrayIcon::Context)
//        emit showWindows();
}

void Notifier::_q_trayClicked()
{
    if (d->trayNotification) {
        QMetaObject::invokeMethod(d->trayNotification, "clicked");
        d->trayNotification = 0;
    }
}
#endif
