/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include "settings.h"
#include "settings_p.h"
#include "systeminfo.h"

#ifdef Q_OS_MAC
extern bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret);
#endif

ApplicationSettings *wSettings = 0;

QString ApplicationSettingsPrivate::executablePath() const
{
#ifdef Q_OS_MAC
    const QString macDir("/Contents/MacOS");
    const QString appDir = qApp->applicationDirPath();
    if (appDir.endsWith(macDir))
        return appDir.left(appDir.size() - macDir.size());
#endif
#ifdef Q_OS_WIN
    return qApp->applicationFilePath().replace("/", "\\");
#endif
    return qApp->applicationFilePath();
}

ApplicationSettings::ApplicationSettings(QObject *parent)
    : QObject(parent),
    d(new ApplicationSettingsPrivate)
{
    d->settings = new QSettings(this);

    if (openAtLogin())
        setOpenAtLogin(true);

    wSettings = this;
}

bool ApplicationSettings::isMobile() const
{
#ifdef WILINK_EMBEDDED
    return true;
#else
    return false;
#endif
}

QString ApplicationSettings::osType() const
{
    return SystemInfo::osType();
}

/** Returns the name of the audio input device.
 */
QString ApplicationSettings::audioInputDeviceName() const
{
    return d->settings->value("AudioInputDevice").toString();
}

/** Sets the name of the audio input device.
 *
 * @param name
 */
void ApplicationSettings::setAudioInputDeviceName(const QString &name)
{
    if (name != audioInputDeviceName()) {
        d->settings->setValue("AudioInputDevice", name);
        emit audioInputDeviceNameChanged(name);
    }
}

/** Returns the name of the audio output device.
 */
QString ApplicationSettings::audioOutputDeviceName() const
{
    return d->settings->value("AudioOutputDevice").toString();
}

/** Sets the name of the audio output device.
 *
 * @param name
 */
void ApplicationSettings::setAudioOutputDeviceName(const QString &name)
{
    if (name != audioOutputDeviceName()) {
        d->settings->setValue("AudioOutputDevice", name);
        emit audioOutputDeviceNameChanged(name);
    }
}

/** Returns the list of disabled plugins.
 */
QStringList ApplicationSettings::disabledPlugins() const
{
    return d->settings->value("DisabledPlugins").toStringList();
}

/** Sets the list of disabled plugins.
 *
 * @param plugins
 */
void ApplicationSettings::setDisabledPlugins(const QStringList &plugins)
{
    if (plugins != disabledPlugins()) {
        d->settings->setValue("DisabledPlugins", plugins);
        emit disabledPluginsChanged(plugins);
    }
}

/** Returns the directory where downloaded files are stored.
 */
QString ApplicationSettings::downloadsLocation() const
{
    QStringList dirNames = QStringList() << "Downloads" << "Download";
    foreach (const QString &dirName, dirNames)
    {
        QDir downloads(QDir::home().filePath(dirName));
        if (downloads.exists())
            return downloads.absolutePath();
    }
#ifdef Q_OS_WIN
    return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
}

/** Returns the list of enabled plugins.
 */
QStringList ApplicationSettings::enabledPlugins() const
{
    return d->settings->value("EnabledPlugins").toStringList();
}

/** Sets the list of enabled plugins.
 *
 * @param plugins
 */
void ApplicationSettings::setEnabledPlugins(const QStringList &plugins)
{
    if (plugins != enabledPlugins()) {
        d->settings->setValue("EnabledPlugins", plugins);
        emit enabledPluginsChanged(plugins);
    }
}

/** Returns thome HOME url.
 */
QUrl ApplicationSettings::homeUrl() const
{
    return QUrl::fromLocalFile(QDir::homePath());
}

/** Returns whether a notification should be shown for incoming messages.
 */
bool ApplicationSettings::incomingMessageNotification() const
{
    return d->settings->value("IncomingMessageNotification", true).toBool();
}

/** Sets whether a notification should be shown for incoming messages.
 *
 * @param notification
 */
void ApplicationSettings::setIncomingMessageNotification(bool notification)
{
    if (notification != incomingMessageNotification()) {
        d->settings->setValue("IncomingMessageNotification", notification);
        emit incomingMessageNotificationChanged(notification);
    }
}

/** Returns whether to play a sound for incoming messages.
 */
bool ApplicationSettings::incomingMessageSound() const
{
    return d->settings->value("IncomingMessageSound").toBool();
}

/** Sets whether to play a sound for incoming messages.
 */
void ApplicationSettings::setIncomingMessageSound(bool sound)
{
    if (sound != incomingMessageSound()) {
        d->settings->setValue("IncomingMessageSound", sound);
        emit incomingMessageSoundChanged(sound);
    }
}

/** Returns whether the application should be run at login.
 */
bool ApplicationSettings::openAtLogin() const
{
    return d->settings->value("OpenAtLogin", true).toBool();
}

/** Sets whether the application should be run at login.
 *
 * @param run
 */
void ApplicationSettings::setOpenAtLogin(bool run)
{
    const QString appName = qApp->applicationName();
    const QString appPath = d->executablePath();
#if defined(Q_OS_MAC)
    QString script = run ?
        QString("tell application \"System Events\"\n"
            "\tmake login item at end with properties {path:\"%1\"}\n"
            "end tell\n").arg(appPath) :
        QString("tell application \"System Events\"\n"
            "\tdelete login item \"%1\"\n"
            "end tell\n").arg(appName);
    qt_mac_execute_apple_script(script, 0);
#elif defined(Q_OS_WIN)
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (run)
        registry.setValue(appName, appPath);
    else
        registry.remove(appName);
#else
    QDir autostartDir(QDir::home().filePath(".config/autostart"));
    if (autostartDir.exists())
    {
        QFile desktop(autostartDir.filePath(appName + ".desktop"));
        const QString fileName = appName + ".desktop";
        if (run)
        {
            if (desktop.open(QIODevice::WriteOnly))
            {
                QTextStream stream(&desktop);
                stream << "[Desktop Entry]\n";
                stream << QString("Name=%1\n").arg(appName);
                stream << QString("Exec=%1\n").arg(appPath);
                stream << "Type=Application\n";
            }
        }
        else
        {
            desktop.remove();
        }
    }
#endif

    // store preference
    if (run != openAtLogin())
    {
        d->settings->setValue("OpenAtLogin", run);
        emit openAtLoginChanged(run);
    }
}

/** Returns whether to play a sound for outgoing messages.
 */
bool ApplicationSettings::outgoingMessageSound() const
{
    return d->settings->value("OutgoingMessageSound").toBool();
}

/** Sets whether to play a sound for outgoing messages.
 */
void ApplicationSettings::setOutgoingMessageSound(bool sound)
{
    if (sound != outgoingMessageSound()) {
        d->settings->setValue("OutgoingMessageSound", sound);
        emit outgoingMessageSoundChanged(sound);
    }
}

/** Returns true if offline contacts should be displayed.
 */
bool ApplicationSettings::showOfflineContacts() const
{
    return d->settings->value("ShowOfflineContacts", true).toBool();
}

/** Sets whether offline contacts should be displayed.
 *
 * @param show
 */
void ApplicationSettings::setShowOfflineContacts(bool show)
{
    if (show != showOfflineContacts()) {
        d->settings->setValue("ShowOfflineContacts", show);
        emit showOfflineContactsChanged(show);
    }
}

/** Returns true if offline contacts should be displayed.
 */
bool ApplicationSettings::sortContactsByStatus() const
{
    return d->settings->value("SortContactsByStatus", true).toBool();
}

/** Sets whether contacts should be sorted by status.
 *
 * @param sort
 */
void ApplicationSettings::setSortContactsByStatus(bool sort)
{
    if (sort != sortContactsByStatus())
    {
        d->settings->setValue("SortContactsByStatus", sort);
        emit sortContactsByStatusChanged(sort);
    }
}

