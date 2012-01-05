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

#include <QAudioOutput>
#include <QIODevice>
#include <QMap>
#include <QPair>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

class QSoundPlayerPrivate
{
public:
    QSoundPlayerPrivate();
    QString inputName;
    QString outputName;
    QMap<int, QAudioOutput*> outputs;
    QMap<int, QSoundFile*> readers;
    QNetworkAccessManager *network;
    int readerId;
};

QSoundPlayerPrivate::QSoundPlayerPrivate()
    : network(0),
    readerId(0)
{
}

QSoundPlayer::QSoundPlayer(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAudioDeviceInfo>();
    d = new QSoundPlayerPrivate;
}

QSoundPlayer::~QSoundPlayer()
{
    delete d;
}

int QSoundPlayer::play(const QString &name, bool repeat)
{
    bool check;
    Q_UNUSED(check);

    if (name.isEmpty())
        return 0;

    QUrl url(name);
    if (url.scheme() == "http" || url.scheme() == "ftp") {
        qWarning("requested network play: %s", qPrintable(name));
        if (d->network) {
            QNetworkReply *reply = d->network->get(QNetworkRequest(url));
            reply->setProperty("_request_url", url);
            check = connect(reply, SIGNAL(finished()),
                            this, SLOT(_q_networkFinished()));
            Q_ASSERT(check);
        }
        return 0;
    }

    QSoundFile *reader = new QSoundFile(name);
    reader->setRepeat(repeat);
    return play(reader);
}

int QSoundPlayer::play(QSoundFile *reader)
{
    if (!reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        delete reader;
        return 0;
    }

    // register reader
    const int id = ++d->readerId;
    d->readers[id] = reader;

    // move reader to audio thread
    reader->setParent(0);
    reader->moveToThread(thread());
    reader->setParent(this);

    // schedule play
    QMetaObject::invokeMethod(this, "_q_start", Q_ARG(int, id));

    return id;
}

QAudioDeviceInfo QSoundPlayer::inputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == d->inputName)
            return info;
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

void QSoundPlayer::setInputDeviceName(const QString &name)
{
    d->inputName = name;
}

QStringList QSoundPlayer::inputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        names << info.deviceName();
    return names;
}

QAudioDeviceInfo QSoundPlayer::outputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.deviceName() == d->outputName)
            return info;
    }
    return QAudioDeviceInfo::defaultOutputDevice();
}

void QSoundPlayer::setOutputDeviceName(const QString &name)
{
    d->outputName = name;
}

QStringList QSoundPlayer::outputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        names << info.deviceName();
    return names;
}

void QSoundPlayer::setNetworkAccessManager(QNetworkAccessManager *network)
{
    d->network = network;
}

void QSoundPlayer::stop(int id)
{
    // schedule stop
    QMetaObject::invokeMethod(this, "_q_stop", Q_ARG(int, id));
}

void QSoundPlayer::_q_networkFinished()
{
    qDebug("network request finished");
}

void QSoundPlayer::_q_start(int id)
{
    QSoundFile *reader = d->readers.value(id);
    if (!reader)
        return;

    QAudioOutput *output = new QAudioOutput(outputDevice(), reader->format(), this);
    connect(output, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(_q_stateChanged(QAudio::State)));
    d->outputs.insert(id, output);
    output->start(reader);
}

void QSoundPlayer::_q_stop(int id)
{
    QAudioOutput *output = d->outputs.value(id);
    if (!output)
        return;

    output->stop();
}

void QSoundPlayer::_q_stateChanged(QAudio::State state)
{
    QAudioOutput *output = qobject_cast<QAudioOutput*>(sender());
    if (!output || state == QAudio::ActiveState)
        return;

    // delete output
    int id = d->outputs.key(output);
    d->outputs.take(id);
    output->deleteLater();

    // delete reader
    QSoundFile *reader = d->readers.take(id);
    if (reader)
        reader->deleteLater();

    emit finished(id);
}

