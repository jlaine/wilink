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

#ifndef __WILINK_SOUND_PLAYER_H__
#define __WILINK_SOUND_PLAYER_H__

#include <QAudioDeviceInfo>
#include <QStringList>
#include <QUrl>

class QNetworkAccessManager;
class QSoundFile;
class QSoundPlayer;
class QSoundPlayerPrivate;
class QSoundPlayerJobPrivate;

class QSoundPlayerJob : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QUrl url READ url CONSTANT)

public:
    enum State {
        IdleState = 0,
        DownloadingState = 1,
        PlayingState = 2,
    };

    int id() const;
    QSoundPlayerJob::State state() const;
    QUrl url() const;

signals:
    void finished();
    void stateChanged();

public slots:
    void stop();

private slots:
    void _q_download();
    void _q_downloadFinished();
    void _q_start();
    void _q_stateChanged(QAudio::State state);

private:
    QSoundPlayerJob(QSoundPlayer *player, int id);
    ~QSoundPlayerJob();

    void setFile(QSoundFile *soundFile);

    QSoundPlayerJobPrivate *d;
    friend class QSoundPlayer;
};

class QSoundPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputDeviceName READ inputDeviceName WRITE setInputDeviceName NOTIFY inputDeviceNameChanged)
    Q_PROPERTY(QString outputDeviceName READ outputDeviceName WRITE setOutputDeviceName NOTIFY outputDeviceNameChanged)
    Q_PROPERTY(QStringList inputDeviceNames READ inputDeviceNames CONSTANT)
    Q_PROPERTY(QStringList outputDeviceNames READ outputDeviceNames CONSTANT)

public:
    QSoundPlayer(QObject *parent = 0);
    ~QSoundPlayer();

    static QSoundPlayer *instance();

    QString inputDeviceName() const;
    void setInputDeviceName(const QString &name);

    QString outputDeviceName() const;
    void setOutputDeviceName(const QString &name);

    QAudioDeviceInfo inputDevice() const;
    QStringList inputDeviceNames() const;

    QAudioDeviceInfo outputDevice() const;
    QStringList outputDeviceNames() const;

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *manager);

signals:
    void finished(int id);
    void inputDeviceNameChanged();
    void outputDeviceNameChanged();

public slots:
    QSoundPlayerJob *play(const QUrl &url, bool repeat = false);
    void stop(int id);

private slots:
    void _q_finished();

private:
    QSoundPlayerPrivate *d;
};

#endif
