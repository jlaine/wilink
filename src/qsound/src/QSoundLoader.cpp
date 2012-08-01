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

#include <QtDeclarative>
#include <QApplication>
#include <QDeclarativeEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "QSoundFile.h"
#include "QSoundLoader.h"
#include "QSoundPlayer.h"

#define SOUNDLOADER_MAXIMUM_REDIRECT_RECURSION 16

class QSoundLoaderPrivate
{
public:
    QSoundLoaderPrivate(QSoundLoader *qq);
    void download(const QUrl &url, QNetworkAccessManager *manager);
    void finish();
    void play(QSoundFile *soundFile);

    QAudioOutput *audioOutput;
    int redirectCount;
    bool repeat;
    QNetworkReply *reply;
    QUrl source;
    QSoundLoader::Status status;

private:
    QSoundLoader *q;
};

QSoundLoaderPrivate::QSoundLoaderPrivate(QSoundLoader *qq)
    : audioOutput(0)
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

void QSoundLoaderPrivate::finish()
{
}

void QSoundLoaderPrivate::play(QSoundFile *soundFile)
{
    bool check;
    Q_UNUSED(check);

    qmlInfo(q) << "Play";

    // open sound file
    soundFile->setRepeat(repeat);
    if (!soundFile->open(QIODevice::Unbuffered | QIODevice::ReadOnly)) {
        qmlInfo(q) << "Could not open sound file";
        finish();
        return;
    }

    // play file
    qmlInfo(q) << "Starting audio";
    QSoundPlayer *player = QSoundPlayer::instance();
    if (!player) {
        qmlInfo(q) << "Could not open find sound player";
        finish();
        return;
    }

    audioOutput = new QAudioOutput(player->outputDevice(), soundFile->format());

    check = QObject::connect(audioOutput, SIGNAL(stateChanged(QAudio::State)),
                             q, SLOT(_q_stateChanged()));
    Q_ASSERT(check);

    audioOutput->moveToThread(player->soundThread());
    soundFile->moveToThread(player->soundThread());
    soundFile->setParent(audioOutput);
    audioOutput->start(soundFile);

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
    delete d;
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

    // clear old

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
        QSoundFile::FileType fileType = QSoundFile::typeFromFileName(d->source.path());
        qmlInfo(this) << "Got sound: \"" << d->source.toString() << '"' << fileType;
        QSoundFile *soundFile = new QSoundFile(d->reply, fileType);
        qmlInfo(this) << "Got sound2: \"" << d->source.toString() << '"' << fileType;
        //d->play(new QSoundFile(d->reply, fileType));
    } else {
        qmlInfo(this) << "Cannot fetch sound: \"" << d->source.toString() << '"';
        d->status = Error;
        emit statusChanged();
        
        d->reply->deleteLater();
        d->reply = 0;
    }
}
