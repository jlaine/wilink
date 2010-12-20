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

#include <QAudioOutput>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

QSoundPlayer::QSoundPlayer(QObject *parent)
    : QObject(parent),
    m_readerId(0)
{
}

int QSoundPlayer::play(const QString &name, bool repeat)
{
    QSoundFile *reader = new QSoundFile(name, this);
    reader->setRepeat(repeat);
    return play(reader);
}

int QSoundPlayer::play(QSoundFile *reader)
{
    if (!reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        delete reader;
        return -1;
    }
    connect(reader, SIGNAL(finished()), this, SLOT(readerFinished()));
    m_readerId++;
    m_readers[m_readerId] = reader;

    QAudioOutput *output = new QAudioOutput(reader->format(), reader);
    output->start(reader);

    return m_readerId;
}

void QSoundPlayer::stop(int id)
{
    QSoundFile *reader = m_readers.value(id);
    if (reader)
        reader->close();
}

void QSoundPlayer::readerFinished()
{
    QSoundFile *reader = qobject_cast<QSoundFile*>(sender());
    if (!reader)
        return;

    int id = m_readers.key(reader);
    m_readers.take(id);
    reader->deleteLater();
}

