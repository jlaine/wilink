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

#include <QAudioFormat>

#include "QSoundMeter.h"

QSoundMeter::QSoundMeter(const QAudioFormat &format, QObject *parent)
    : QIODevice(parent),
    m_device(0)
{
}

qint64 QSoundMeter::pos() const
{
    if (!m_device)
        return 0;
    return m_device->pos();
}

qint64 QSoundMeter::readData(char *data, qint64 maxSize)
{
    if (!m_device)
        return -1;
    qint64 length = m_device->read(data, maxSize);
    return length;
}

bool QSoundMeter::seek(qint64 pos)
{
    if (!m_device)
        return false;
    return m_device->seek(pos);
}

qint64 QSoundMeter::writeData(const char *data, qint64 maxSize)
{
    if (!m_device)
        return -1;
    qint64 length = m_device->write(data, maxSize);
    return length;
}

