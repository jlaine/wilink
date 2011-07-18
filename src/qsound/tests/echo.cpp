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

#include <cstdlib>
#include <csignal>

#include <QApplication>
#include <QBuffer>
#include <QTimer>

#include "QSoundFile.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"
#include "echo.h"

EchoTester::EchoTester(QObject *parent)
    : QObject(parent)
{
    m_file = new QSoundFile("test.wav", this);

    m_player = new QSoundPlayer(this);

    m_stream = new QSoundStream(m_player);
    m_stream->setFormat(1, 8000);

    m_file->setFormat(m_stream->format());
    m_file->open(QIODevice::WriteOnly);

    m_stream->setDevice(m_file);
    m_stream->startInput();

    QTimer::singleShot(5000, qApp, SLOT(quit()));
}

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    /* Install signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    EchoTester tester;

    return app.exec();
}
