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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

class ApplicationSettingsPrivate;

/** The ApplicationSettings class represents the Application's settings.
 */
class ApplicationSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString audioInputDeviceName READ audioInputDeviceName WRITE setAudioInputDeviceName NOTIFY audioInputDeviceNameChanged)
    Q_PROPERTY(QString audioOutputDeviceName READ audioOutputDeviceName WRITE setAudioOutputDeviceName NOTIFY audioOutputDeviceNameChanged)
    Q_PROPERTY(QStringList disabledPlugins READ disabledPlugins WRITE setDisabledPlugins NOTIFY disabledPluginsChanged)
    Q_PROPERTY(QString downloadsLocation READ downloadsLocation CONSTANT)
    Q_PROPERTY(QStringList enabledPlugins READ enabledPlugins WRITE setEnabledPlugins NOTIFY enabledPluginsChanged)
    Q_PROPERTY(QUrl homeUrl READ homeUrl CONSTANT)
    Q_PROPERTY(bool incomingMessageNotification READ incomingMessageNotification WRITE setIncomingMessageNotification NOTIFY incomingMessageNotificationChanged)
    Q_PROPERTY(QString incomingMessageSound READ incomingMessageSound WRITE setIncomingMessageSound NOTIFY incomingMessageSoundChanged)
    Q_PROPERTY(bool openAtLogin READ openAtLogin WRITE setOpenAtLogin NOTIFY openAtLoginChanged)
    Q_PROPERTY(QString outgoingMessageSound READ outgoingMessageSound WRITE setOutgoingMessageSound NOTIFY outgoingMessageSoundChanged)
    Q_PROPERTY(bool sharesConfigured READ sharesConfigured WRITE setSharesConfigured NOTIFY sharesConfiguredChanged)
    Q_PROPERTY(QVariantList sharesDirectories READ sharesDirectories WRITE setSharesDirectories NOTIFY sharesDirectoriesChanged)
    Q_PROPERTY(QString sharesLocation READ sharesLocation WRITE setSharesLocation NOTIFY sharesLocationChanged)
    Q_PROPERTY(QUrl sharesUrl READ sharesUrl NOTIFY sharesLocationChanged)
    Q_PROPERTY(bool showOfflineContacts READ showOfflineContacts WRITE setShowOfflineContacts NOTIFY showOfflineContactsChanged)
    Q_PROPERTY(bool sortContactsByStatus READ sortContactsByStatus WRITE setSortContactsByStatus NOTIFY sortContactsByStatusChanged)

public:
    ApplicationSettings(QObject *parent = 0);

    QString audioInputDeviceName() const;
    void setAudioInputDeviceName(const QString &name);

    QString audioOutputDeviceName() const;
    void setAudioOutputDeviceName(const QString &name);

    QStringList disabledPlugins() const;
    void setDisabledPlugins(const QStringList &plugins);

    QString downloadsLocation() const;

    QStringList enabledPlugins() const;
    void setEnabledPlugins(const QStringList &plugins);

    QUrl homeUrl() const;

    bool incomingMessageNotification() const;
    void setIncomingMessageNotification(bool notification);

    QString incomingMessageSound() const;
    void setIncomingMessageSound(const QString &soundFile);

    bool openAtLogin() const;
    void setOpenAtLogin(bool run);

    QString outgoingMessageSound() const;
    void setOutgoingMessageSound(const QString &soundFile);

    bool sharesConfigured() const;
    void setSharesConfigured(bool configured);

    QVariantList sharesDirectories() const;
    void setSharesDirectories(const QVariantList &directories);

    QString sharesLocation() const;
    void setSharesLocation(const QString &location);
    QUrl sharesUrl() const;

    bool showOfflineContacts() const;
    void setShowOfflineContacts(bool show);

    bool sortContactsByStatus() const;
    void setSortContactsByStatus(bool sort);

signals:
    /// \cond
    void audioInputDeviceNameChanged(const QString &name);
    void audioOutputDeviceNameChanged(const QString &name);
    void disabledPluginsChanged(const QStringList &plugins);
    void enabledPluginsChanged(const QStringList &plugins);
    void incomingMessageNotificationChanged(bool notification);
    void incomingMessageSoundChanged(const QString &sound);
    void openAtLoginChanged(bool run);
    void outgoingMessageSoundChanged(const QString &sound);
    void sharesConfiguredChanged(bool configured);
    void sharesDirectoriesChanged(const QVariantList &directories);
    void sharesLocationChanged(const QString &location);
    void showOfflineContactsChanged(bool show);
    void sortContactsByStatusChanged(bool sort);
    /// \endcond

private:
    ApplicationSettingsPrivate *d;
};

extern ApplicationSettings *wSettings;

#endif