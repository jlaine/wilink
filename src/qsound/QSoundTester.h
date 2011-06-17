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

#ifndef __WILINK_SOUND_TESTER_H__
#define __WILINK_SOUND_TESTER_H__

#include <QAudioDeviceInfo>
#include <QMap>
#include <QObject>

class QAudioInput;
class QAudioOutput;
class QBuffer;

class QSoundTester : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(int duration READ duration CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)


public:
    enum State {
        IdleState = 0,
        RecordingState,
        PlayingState,
    };

    QSoundTester(QObject *parent = 0);
    int duration() const;
    State state() const;
    int volume() const;
    int maximumVolume() const;

signals:
    void stateChanged(State state);
    void volumeChanged(int volume);

public slots:
    void start(const QString &inputDevice, const QString &outputDevice);

private slots:
    void _q_playback();
    void _q_stop();
    void _q_volumeChanged(int volume);

private:
    QBuffer *m_buffer;
    QAudioInput *m_input;
    QAudioOutput *m_output;
    State m_state;
    int m_volume;
};

#endif
