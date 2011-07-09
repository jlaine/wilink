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

#include <QImage>
#include <QObject>

class QXmppVideoFrame;
class QVideoGrabber;

class Grabber : public QObject
{
    Q_OBJECT

public:
    Grabber();
    void start(QVideoGrabber *input);

signals:
    void finished();

private slots:
    void saveFrame(const QXmppVideoFrame &frame);

private:
    int m_count;
    QImage m_image;
    QVideoGrabber *m_input;
};

