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

#ifndef __WILINK_SOUND_STREAM_H__
#define __WILINK_SOUND_STREAM_H__

#include <QIODevice>

class QAudioFormat;
class QSoundPlayer;
class QSoundStreamPrivate;

class QSoundStream : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)

public:
    QSoundStream(QSoundPlayer *player);
    ~QSoundStream();

    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;

    void startInput(const QAudioFormat &format, QIODevice *device);
    void stopInput();

    void startOutput(const QAudioFormat &format, QIODevice *device);
    void stopOutput();

    static QAudioFormat pcmAudioFormat(unsigned char channels, unsigned int clockrate);

signals:
    // This signal is emitted when the input volume changes.
    void inputVolumeChanged(int volume);

    // This signal is emitted when the output volume changes.
    void outputVolumeChanged(int volume);

private slots:
    void _q_audioInputStateChanged();
    void _q_audioOutputStateChanged();
    void _q_startInput(const QAudioFormat &format, QIODevice *device);
    void _q_startOutput(const QAudioFormat &format, QIODevice *device);

private:
    QSoundStreamPrivate *d;
};

#endif
