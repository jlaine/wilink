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

#include <QAbstractNetworkCache>
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

class QSoundPlayerJob
{
public:
    QSoundPlayerJob();
    ~QSoundPlayerJob();

    QAudioOutput *audioOutput;
    QNetworkReply *networkReply;
    QSoundFile *reader;
    bool repeat;
    QUrl url;
};

QSoundPlayerJob::QSoundPlayerJob()
    : audioOutput(0),
    networkReply(0),
    reader(0),
    repeat(false)
{
}

QSoundPlayerJob::~QSoundPlayerJob() 
{
    if (networkReply)
        networkReply->deleteLater();
    if (audioOutput)
        audioOutput->deleteLater();
    if (reader)
        reader->deleteLater();
}

class QSoundPlayerPrivate
{
public:
    QSoundPlayerPrivate();
    QString inputName;
    QString outputName;
    QMap<int, QSoundPlayerJob*> jobs;
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
    if (name.isEmpty())
        return 0;

    QUrl url(name);
    if (url.scheme() == "http" || url.scheme() == "ftp") {
        qWarning("requested network play: %s", qPrintable(name));
        if (d->network) {
            const int id = ++d->readerId;
            d->jobs[id] = new QSoundPlayerJob;
            d->jobs[id]->repeat = repeat;
            d->jobs[id]->url = url;
            QMetaObject::invokeMethod(this, "_q_download", Q_ARG(int, id));
            return id;
        } else {
            qWarning("No network access manager to download sound file.");
            return 0;
        }
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
    d->jobs[id] = new QSoundPlayerJob;
    d->jobs[id]->reader = reader;

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

void QSoundPlayer::_q_download(int id)
{
    bool check;
    Q_UNUSED(check);

    QSoundPlayerJob *job = d->jobs.value(id);
    if (!job)
        return;

    job->networkReply = d->network->get(QNetworkRequest(job->url));
    check = connect(job->networkReply, SIGNAL(finished()),
                    this, SLOT(_q_downloadFinished()));
    Q_ASSERT(check);
}

void QSoundPlayer::_q_downloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;

    qDebug("network request finished");
    foreach (QSoundPlayerJob *job, d->jobs.values()) {
        if (job->networkReply == reply) {
            const int id = d->jobs.key(job);
            qDebug("found job %i", id);

            QIODevice *iodevice = d->network->cache()->data(reply->url());
            job->reader = new QSoundFile(iodevice, QSoundFile::Mp3File, this);
            job->reader->setRepeat(job->repeat);

            if (job->reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly))
                _q_start(id);
            break;
        }
    }
}

void QSoundPlayer::_q_start(int id)
{
    QSoundPlayerJob *job = d->jobs.value(id);
    if (!job || !job->reader)
        return;

    qDebug("starting audio for job %i", id);
    job->audioOutput = new QAudioOutput(outputDevice(), job->reader->format(), this);
    connect(job->audioOutput, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(_q_stateChanged(QAudio::State)));
    job->audioOutput->start(job->reader);
}

void QSoundPlayer::_q_stop(int id)
{
    QSoundPlayerJob *job = d->jobs.value(id);
    if (!job || !job->audioOutput)
        return;

    job->audioOutput->stop();
}

void QSoundPlayer::_q_stateChanged(QAudio::State state)
{
    QAudioOutput *output = qobject_cast<QAudioOutput*>(sender());
    if (!output || state == QAudio::ActiveState)
        return;

    foreach (QSoundPlayerJob *job, d->jobs.values()) {
        if (job->audioOutput == output) {
            const int id = d->jobs.key(job);
            qDebug("audio stopped for job %i", id);

            d->jobs.remove(id);
            delete job;

            emit finished(id);
            break;
        }
    }
}

