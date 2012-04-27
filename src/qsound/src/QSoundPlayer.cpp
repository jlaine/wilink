/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

static QSoundPlayer *thePlayer = 0;

class QSoundPlayerJobPrivate
{
public:
    QSoundPlayerJobPrivate(QSoundPlayerJob *qq);
    void finish();

    int id;
    QAudioOutput *audioOutput;
    QNetworkReply *networkReply;
    QSoundPlayer *player;
    QSoundFile *reader;
    bool repeat;
    QSoundPlayerJob::State state;
    QUrl url;

private:
    QSoundPlayerJob *q;
};

QSoundPlayerJobPrivate::QSoundPlayerJobPrivate(QSoundPlayerJob *qq)
    : id(0),
    audioOutput(0),
    networkReply(0),
    player(0),
    reader(0),
    repeat(false),
    state(QSoundPlayerJob::IdleState),
    q(qq)
{
}

void QSoundPlayerJobPrivate::finish()
{
    state = QSoundPlayerJob::IdleState;
    QMetaObject::invokeMethod(q, "stateChanged");
    QMetaObject::invokeMethod(q, "finished");
}

QSoundPlayerJob::QSoundPlayerJob(QSoundPlayer *player, int id)
{
    d = new QSoundPlayerJobPrivate(this);
    d->id = id;
    d->player = player;

    moveToThread(player->thread());
    setParent(player);
}

QSoundPlayerJob::~QSoundPlayerJob() 
{
    if (d->networkReply)
        d->networkReply->deleteLater();
    if (d->audioOutput)
        d->audioOutput->deleteLater();
    if (d->reader)
        d->reader->deleteLater();
    delete d;
}

int QSoundPlayerJob::id() const
{
    return d->id;
}

QSoundPlayerJob::State QSoundPlayerJob::state() const
{
    return d->state;
}

QUrl QSoundPlayerJob::url() const
{
    return d->url;
}

void QSoundPlayerJob::setFile(QSoundFile *soundFile)
{
    d->reader = soundFile;
    d->reader->setRepeat(d->repeat);

    // move reader to audio thread
    d->reader->setParent(0);
    d->reader->moveToThread(thread());
    d->reader->setParent(this);

    if (d->reader->open(QIODevice::Unbuffered | QIODevice::ReadOnly))
        QMetaObject::invokeMethod(this, "_q_start");
    else {
        qWarning("QSoundPlayer(%i) could not open sound file", d->id);
        d->finish();
    }
}

void QSoundPlayerJob::stop()
{
    if (d->networkReply)
        d->networkReply->abort();
    if (d->audioOutput)
        d->audioOutput->stop();
}

void QSoundPlayerJob::_q_download()
{
    bool check;
    Q_UNUSED(check);

    qDebug("QSoundPlayer(%i) requesting %s", d->id, qPrintable(d->url.toString()));
    d->networkReply = d->player->networkAccessManager()->get(QNetworkRequest(d->url));
    check = connect(d->networkReply, SIGNAL(finished()),
                    this, SLOT(_q_downloadFinished()));
    Q_ASSERT(check);

    d->state = DownloadingState;
    emit stateChanged();
}

void QSoundPlayerJob::_q_downloadFinished()
{
    bool check;
    Q_UNUSED(check);

    QNetworkReply *reply = d->networkReply;
    reply->deleteLater();
    d->networkReply = 0;

    // follow redirect
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        redirectUrl = reply->url().resolved(redirectUrl);

        qDebug("QSoundPlayer(%i) following redirect to %s", d->id, qPrintable(redirectUrl.toString()));
        d->networkReply = d->player->networkAccessManager()->get(QNetworkRequest(redirectUrl));
        check = connect(d->networkReply, SIGNAL(finished()),
                        this, SLOT(_q_downloadFinished()));
        Q_ASSERT(check);
        return;
    }

    // check reply
    if (reply->error() != QNetworkReply::NoError) {
        qWarning("QSoundPlayer(%i) failed to retrieve file: %s", d->id, qPrintable(reply->errorString()));
        d->finish();
        return;
    }

    // read file
    QSoundFile::FileType fileType = QSoundFile::UnknownFile;
    QAbstractNetworkCache *cache = d->player->networkAccessManager()->cache();
    const QNetworkCacheMetaData::RawHeaderList headers = cache->metaData(reply->url()).rawHeaders();
    foreach (const QNetworkCacheMetaData::RawHeader &header, headers) {
        if (header.first.toLower() == "content-type")
            fileType = QSoundFile::typeFromMimeType(header.second);
    }

    QIODevice *iodevice = d->player->networkAccessManager()->cache()->data(reply->url());
    setFile(new QSoundFile(iodevice, fileType));
}

void QSoundPlayerJob::_q_start()
{
    bool check;
    Q_UNUSED(check);

    if (!d->reader)
        return;

    qDebug("QSoundPlayer(%i) starting audio", d->id);
    d->audioOutput = new QAudioOutput(d->player->outputDevice(), d->reader->format(), this);
    check = connect(d->audioOutput, SIGNAL(stateChanged(QAudio::State)),
                    this, SLOT(_q_stateChanged(QAudio::State)));
    Q_ASSERT(check);

    d->audioOutput->start(d->reader);

    d->state = PlayingState;
    emit stateChanged();
}

void QSoundPlayerJob::_q_stateChanged(QAudio::State state)
{
    if (state != QAudio::ActiveState) {
        qDebug("QSoundPlayer(%i) audio stopped", d->id);
        d->finish();
    }
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

    thePlayer = this;
}

QSoundPlayer::~QSoundPlayer()
{
    thePlayer = 0;
    delete d;
}

QSoundPlayer* QSoundPlayer::instance()
{
    return thePlayer;
}

QSoundPlayerJob *QSoundPlayer::play(const QUrl &url, bool repeat)
{
    if (!url.isValid())
        return 0;

    QSoundPlayerJob *job = new QSoundPlayerJob(this, ++d->readerId);
    job->d->repeat = repeat;
    job->d->url = url;
    d->jobs[job->id()] = job;

    if (url.scheme() == "file") {
        job->setFile(new QSoundFile(url.toLocalFile()));
    } else if (url.scheme() == "qrc" || url.scheme() == "") {
        const QString path = QLatin1String(":") + (url.path().startsWith("/") ? "" : "/") + url.path();
        job->setFile(new QSoundFile(path));
    } else if (d->network) {
        QMetaObject::invokeMethod(job, "_q_download");
    } else {
        QMetaObject::invokeMethod(job, "finished", Qt::QueuedConnection);
    }
    return job;
}

QAudioDeviceInfo QSoundPlayer::inputDevice() const
{
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.deviceName() == d->inputName)
            return info;
    }
    return QAudioDeviceInfo::defaultInputDevice();
}

QString QSoundPlayer::inputDeviceName() const
{
    return d->inputName;
}

void QSoundPlayer::setInputDeviceName(const QString &name)
{
    if (name != d->inputName) {
        d->inputName = name;
        emit inputDeviceNameChanged();
    }
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

QString QSoundPlayer::outputDeviceName() const
{
    return d->outputName;
}

void QSoundPlayer::setOutputDeviceName(const QString &name)
{
    if (name != d->outputName) {
        d->outputName = name;
        emit outputDeviceNameChanged();
    }
}

QStringList QSoundPlayer::outputDeviceNames() const
{
    QStringList names;
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        names << info.deviceName();
    return names;
}

QNetworkAccessManager *QSoundPlayer::networkAccessManager() const
{
    return d->network;
}

void QSoundPlayer::setNetworkAccessManager(QNetworkAccessManager *network)
{
    d->network = network;
}

void QSoundPlayer::stop(int id)
{
    QSoundPlayerJob *job = d->jobs.value(id);
    if (job)
        QMetaObject::invokeMethod(job, "stop");
}

void QSoundPlayer::_q_finished()
{
    QSoundPlayerJob *job = qobject_cast<QSoundPlayerJob*>(sender());
    if (!job)
        return;

    d->jobs.take(job->id());
    job->deleteLater();
    emit finished(job->id());
}

