/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QTranslator>

#include "translations.h"

#define TRANSLATIONLOADER_MAXIMUM_REDIRECT_RECURSION 16

class TranslationLoaderPrivate
{
public:
    TranslationLoaderPrivate(TranslationLoader *qq);
    void download(const QUrl &url, QNetworkAccessManager *manager);

    QByteArray data;
    int redirectCount;
    QNetworkReply *reply;
    QUrl source;
    TranslationLoader::Status status;
    QTranslator *translator;
    bool translatorInstalled;

private:
    TranslationLoader *q;
};

TranslationLoaderPrivate::TranslationLoaderPrivate(TranslationLoader *qq)
    : redirectCount(0)
    , reply(0)
    , status(TranslationLoader::Null)
    , translator(0)
    , translatorInstalled(0)
    , q(qq)
{
}

void TranslationLoaderPrivate::download(const QUrl &url, QNetworkAccessManager *manager)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    reply = manager->get(req);
    QObject::connect(reply, SIGNAL(finished()), q, SLOT(_q_replyFinished()));
}

TranslationLoader::TranslationLoader(QObject *parent)
    : QObject(parent)
    , d(new TranslationLoaderPrivate(this))
{
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

    if (d->translatorInstalled) {
        qApp->removeTranslator(d->translator);
        d->translatorInstalled = false;
    }

    d->download(d->source, qmlEngine(this)->networkAccessManager());

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

    d->redirectCount++;
    if (d->redirectCount < TRANSLATIONLOADER_MAXIMUM_REDIRECT_RECURSION) {
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
        d->data = d->reply->readAll();
        if (!d->translator)
            d->translator = new QTranslator(this);
        if (d->translator->load((const uchar*)d->data.constData(), d->data.size())) {
            qApp->installTranslator(d->translator);
            d->translatorInstalled = true;
            d->status = Ready;
        } else {
            qmlInfo(this) << "Cannot load translation: \"" << d->source.toString() << '"';
            d->status = Error;
        }
    } else {
        qmlInfo(this) << "Cannot fetch translation: \"" << d->source.toString() << '"';
        d->status = Error;
    }

    emit statusChanged();

    d->reply->deleteLater();
    d->reply = 0;
}
