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

#ifndef __WILINK_SOUNDS_H__
#define __WILINK_SOUNDS_H__

#include <QObject>
#include <QUrl>

class QAudioOutput;
class QSoundFile;
class QSoundLoaderPrivate;

class QSoundLoader : public QObject
{
    Q_OBJECT
    Q_ENUMS(Status)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status { Null = 0, Playing, Loading, Error };

    QSoundLoader(QObject *parent = 0);
    ~QSoundLoader();

    bool repeat() const;
    void setRepeat(bool repeat);

    QUrl source() const;
    void setSource(const QUrl &source);

    Status status() const;

signals:
    void repeatChanged();
    void sourceChanged();
    void statusChanged();

public slots:
    void start();
    void stop();

private slots:
    void _q_replyFinished();
    void _q_soundFinished();

private:
    QSoundLoaderPrivate *d;
    friend class QSoundLoaderPrivate;
};

class QSoundJob : public QObject
{
    Q_OBJECT

public:
    QSoundJob(QSoundFile *soundFile);

signals:
    void finished();

public slots:
    void start();
    void stop();

private slots:
    void _q_stateChanged();

private:
    QSoundFile* m_file;
    QAudioOutput* m_output;
};

#endif
