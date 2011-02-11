/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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
#include <QAudioOutput>
#include <QIODevice>
#include <QMap>
#include <QPair>

class QFile;
class QSoundFile;
class QSoundFilePrivate;

class QSoundPlayer : public QObject
{
    Q_OBJECT

public:
    QSoundPlayer(QObject *parent = 0);
    int play(const QString &name, bool repeat = false);
    int play(QSoundFile *reader);
    void stop(int id);

signals:
    void finished(int id);

public slots:
    void setAudioOutputDevice(const QAudioDeviceInfo &audioDevice);

private slots:
    void stateChanged(QAudio::State state);

private:
    QAudioDeviceInfo m_audioDevice;
    int m_readerId;
    QMap<int, QSoundFile*> m_readers;
};

#endif
