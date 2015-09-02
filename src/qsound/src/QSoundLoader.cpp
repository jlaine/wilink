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

#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlInfo>

#include "QSoundFile.h"
#include "QSoundLoader.h"
#include "QSoundPlayer.h"

#define SOUNDLOADER_MAXIMUM_REDIRECT_RECURSION 16

class QSoundLoaderPrivate
{
public:
    QSoundLoaderPrivate(QSoundLoader *qq);
    void download(const QUrl &url, QNetworkAccessManager *manager);
    void play(QSoundFile *soundFile);

    QSoundJob *audioJob;
    int redirectCount;
    bool repeat;
    QNetworkReply *reply;
    QUrl source;
    QSoundLoader::Status status;

private:
    QSoundLoader *q;
};

QSoundLoaderPrivate::QSoundLoaderPrivate(QSoundLoader *qq)
    : audioJob(0)
    , redirectCount(0)
    , repeat(false)
    , reply(0)
    , status(QSoundLoader::Null)
    , q(qq)
{
}

void QSoundLoaderPrivate::download(const QUrl &url, QNetworkAccessManager *manager)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    reply = manager->get(req);
    QObject::connect(reply, SIGNAL(finished()), q, SLOT(_q_replyFinished()));
}

void QSoundLoaderPrivate::play(QSoundFile *soundFile)
{
    // open sound file
    soundFile->setRepeat(repeat);
    if (!soundFile->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        qmlInfo(q) << "Could not open sound file";
        delete soundFile;
        status = QSoundLoader::Error;
        emit q->statusChanged();
        return;
    }

    // play file
    qmlInfo(q) << "Audio started";
    audioJob = new QSoundJob(soundFile);
    audioJob->moveToThread(QSoundPlayer::instance()->soundThread());
    QObject::connect(audioJob, SIGNAL(finished()),
                     q, SLOT(_q_soundFinished()));
    QMetaObject::invokeMethod(audioJob, "start");

    status = QSoundLoader::Playing;
    emit q->statusChanged();
}

QSoundLoader::QSoundLoader(QObject *parent)
    : QObject(parent)
    , d(new QSoundLoaderPrivate(this))
{
}

QSoundLoader::~QSoundLoader()
{
    stop();
    if (d->reply)
        d->reply->deleteLater();
    if (d->audioJob)
        d->audioJob->deleteLater();
    delete d;
}

bool QSoundLoader::repeat() const
{
    return d->repeat;
}

void QSoundLoader::setRepeat(bool repeat)
{
    if (repeat != d->repeat) {
        d->repeat = repeat;
        emit repeatChanged();
    }
}

QUrl QSoundLoader::source() const
{
    return d->source;
}

void QSoundLoader::setSource(const QUrl &source)
{
    if (source == d->source)
        return;
    d->source = source;
    emit sourceChanged();
}

void QSoundLoader::start()
{
    // TODO: stop old

    // if we have no URL, do nothing
    if (!d->source.isValid())
        return;

    if (d->source.scheme() == "file") {
        d->play(new QSoundFile(d->source.toLocalFile()));
    } else if (d->source.scheme() == "qrc" || d->source.scheme() == "") {
        const QString path = QLatin1String(":") + (d->source.path().startsWith("/") ? "" : "/") + d->source.path();
        d->play(new QSoundFile(path));
    } else {
        d->download(d->source, qmlEngine(this)->networkAccessManager());
        d->status = Loading;
        emit statusChanged();
    }
}

void QSoundLoader::stop()
{
    if (d->reply)
        d->reply->abort();
    if (d->audioJob)
        QMetaObject::invokeMethod(d->audioJob, "stop");
}

QSoundLoader::Status QSoundLoader::status() const
{
    return d->status;
}

void QSoundLoader::_q_replyFinished()
{
    if (!d->reply)
        return;

    d->redirectCount++;
    if (d->redirectCount < SOUNDLOADER_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = d->reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            const QUrl url = d->reply->url().resolved(redirect.toUrl());
            QNetworkAccessManager *manager = d->reply->manager();
            d->reply->deleteLater();
            d->reply = 0;
            d->download(url, manager);
            return;
        }
    }
    d->redirectCount = 0;

    if (d->reply->error() == QNetworkReply::NoError) {
        // detect file type
        const QByteArray contentType = d->reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        QSoundFile::FileType fileType = QSoundFile::typeFromMimeType(contentType);
        if (fileType == QSoundFile::UnknownFile)
            fileType = QSoundFile::typeFromFileName(d->source.path());
        if (fileType == QSoundFile::UnknownFile) {
            qmlInfo(this) << "Cannot determine sound type: \"" << d->source.toString() << '"';
            d->status = Error;
            emit statusChanged();
        }

        // play file
        QBuffer *buffer = new QBuffer(this);
        buffer->setData(d->reply->readAll());
        d->play(new QSoundFile(buffer, fileType));
    } else {
        qmlInfo(this) << "Cannot fetch sound: \"" << d->source.toString() << '"';
        d->status = Error;
        emit statusChanged();
    }

    d->reply->deleteLater();
    d->reply = 0;
}

void QSoundLoader::_q_soundFinished()
{
    if (sender() == d->audioJob) {
        qmlInfo(this) << "Audio stopped";
        d->audioJob = 0;
        d->status = QSoundLoader::Null;
        emit statusChanged();
    }
}

QSoundJob::QSoundJob(QSoundFile *file)
    : m_file(file)
    , m_output(0)
{
    m_file->setParent(this);
}

void QSoundJob::start()
{
    QSoundPlayer *player = QSoundPlayer::instance();
    if (!player) {
        qWarning("Could not find sound player");
        emit finished();
        return;
    }

    m_output = new QAudioOutput(player->outputDevice(), m_file->format(), this);
    connect(m_output, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(_q_stateChanged()));
    m_output->start(m_file);
}

void QSoundJob::stop()
{
    if (m_output)
        m_output->stop();
}

void QSoundJob::_q_stateChanged()
{
    if (m_output && m_output->state() != QAudio::ActiveState) {
        m_output->deleteLater();
        m_output = 0;
        emit finished();
    }
}

