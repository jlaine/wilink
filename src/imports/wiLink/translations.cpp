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
#include <QTranslator>

#include "translations.h"

class TranslationLoaderPrivate
{
public:
    QByteArray data;
    QNetworkReply *reply;
    QUrl source;
    TranslationLoader::Status status;
    QTranslator *translator;
};

TranslationLoader::TranslationLoader(QObject *parent)
    : QObject(parent)
    , d(new TranslationLoaderPrivate)
{
    d->reply = 0;
    d->status = Null;
    d->translator = 0;
}

TranslationLoader::~TranslationLoader()
{
    delete d;
}

QString TranslationLoader::localeName() const
{
    return QLocale::system().name().split("_").first();
}

QUrl TranslationLoader::source() const
{
    return d->source;
}

void TranslationLoader::setSource(const QUrl &source)
{
    if (source == d->source)
        return;

    d->source = source;
    emit sourceChanged();

    QNetworkRequest request(d->source);
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    d->reply = qmlEngine(this)->networkAccessManager()->get(request);
    connect(d->reply, SIGNAL(finished()),
            this, SLOT(_q_replyFinished()));

    d->status = Loading;
    emit statusChanged();
}

TranslationLoader::Status TranslationLoader::status() const
{
    return d->status;
}

void TranslationLoader::_q_replyFinished()
{
    if (!d->reply)
        return;

    if (d->reply->error() == QNetworkReply::NoError) {
        if (d->translator) {
            delete d->translator;
            d->translator = 0;
        }

        d->data = d->reply->readAll();
        d->translator = new QTranslator(this);
        if (d->translator->load((const uchar*)d->data.constData(), d->data.size())) {
            qApp->installTranslator(d->translator);
            d->status = Ready;
            emit statusChanged();
        } else {
            delete d->translator;
            d->translator = 0;
            d->status = Error;
            emit statusChanged();
        }
    } else {
        d->status = Error;
        emit statusChanged();
    }

    d->reply->deleteLater();
    d->reply = 0;
}
