/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <QApplication>
#include <QAuthenticator>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDomDocument>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>

#include "qnetio/wallet.h"

#include "chat.h"
#include "config.h"
#include "diagnostics.h"
#include "photos.h"
#include "trayicon.h"
#include "updates.h"

static const QUrl baseUrl("https://www.wifirst.net/w/");
static const QString authSuffix = "@wifirst.net";

TrayIcon::TrayIcon()
    : chat(NULL), diagnostics(NULL), photos(NULL)
{
    /* set icon */
    setIcon(QIcon(":/wDesktop.png"));
    show();

    /* set initial menu */
    QMenu *menu = new QMenu;
    QAction *action = menu->addAction(QIcon(":/diagnostics.png"), tr("&Diagnostics"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showDiagnostics()));

    action = menu->addAction(QIcon(":/remove.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    setContextMenu(menu);

#ifndef Q_WS_MAC
    /* convert left clicks to right clicks, except on OS X */
    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
#endif

    /* prepare network manager */
    network = new QNetworkAccessManager(this);
    connect(Wallet::instance(), SIGNAL(credentialsRequired(const QString&, QAuthenticator *)), this, SLOT(getCredentials(const QString&, QAuthenticator *)));
    connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(network, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)), Wallet::instance(), SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> &)));

    /* prepare modules */
    chat = new Chat(this);
    updates = new UpdatesDialog;

    /* fetch menu */
    QNetworkRequest req(baseUrl);
    req.setRawHeader("Accept", "application/xml");
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showMenu()));
}

void TrayIcon::fetchIcon()
{
    if (icons.empty())
        return;

    QPair<QUrl, QAction*> entry = icons.first();
    QNetworkReply *reply = network->get(QNetworkRequest(entry.first));
    connect(reply, SIGNAL(finished()), this, SLOT(showIcon()));
}

/** Prompt the user for credentials.
 */
void TrayIcon::getCredentials(const QString &realm, QAuthenticator *authenticator)
{
    const QString prompt = QString("Please enter your credentials for '%1'.").arg(realm);

    /* create dialog */
    QDialog *dialog = new QDialog;
    QGridLayout *layout = new QGridLayout;

    layout->addWidget(new QLabel(prompt), 0, 0, 1, 2);

    layout->addWidget(new QLabel("User:"), 1, 0);
    QLineEdit *user = new QLineEdit();
    layout->addWidget(user, 1, 1);

    layout->addWidget(new QLabel("Password:"), 2, 0);
    QLineEdit *password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    layout->addWidget(password, 2, 1);

    QPushButton *btn = new QPushButton("OK");
    dialog->connect(btn, SIGNAL(clicked()), dialog, SLOT(accept()));
    layout->addWidget(btn, 3, 1);

    dialog->setLayout(layout);

    /* prompt user */
    while (user->text().isEmpty() || password->text().isEmpty())
    {
        if (!dialog->exec())
            return;
    }
    QString userName = user->text();
    if (realm == baseUrl.host() && !userName.endsWith(authSuffix))
        userName += authSuffix;
    authenticator->setUser(userName);
    authenticator->setPassword(password->text());

    /* store credentials */
    QNetIO::Wallet::instance()->setCredentials(realm, userName, password->text());
}

void TrayIcon::openUrl()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        QDesktopServices::openUrl(action->data().toUrl());
}

void TrayIcon::showIcon()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    /* display current icon */
    QPair<QUrl, QAction*> entry = icons.takeFirst();
    QPixmap pixmap;
    QByteArray data = reply->readAll();
    if (!pixmap.loadFromData(data, 0))
    {
        qWarning() << "could not load icon" << entry.first;
        return;
    }
    entry.second->setIcon(QIcon(pixmap));

    /* fetch next icon */
    fetchIcon();
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        // FIXME: this implies that the system tray is at the bottom of the screen..
        QMenu *menu = contextMenu();
        QPoint delta = QPoint(0, menu->sizeHint().height());
        menu->popup(geometry().topLeft() - delta);
    }
}

void TrayIcon::showDiagnostics()
{
    if (!diagnostics)
        diagnostics = new Diagnostics;
    diagnostics->show();
}

void TrayIcon::showMenu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Failed to retrieve menu");
        return;
    }

    QMenu *menu = new QMenu;
    QAction *action;

    /* parse menu */
    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement().firstChildElement("menu").firstChildElement("menu");
    while (!item.isNull())
    {
        const QString image = item.firstChildElement("image").text();
        const QString link = item.firstChildElement("link").text();
        const QString text = item.firstChildElement("label").text();
        if (text.isEmpty())
            menu->addSeparator();
        else {
            action = menu->addAction(text);
            action->setData(baseUrl.resolved(link));
            connect(action, SIGNAL(triggered(bool)), this, SLOT(openUrl()));
            /* schedule retrieval of icon */
            if (!image.isEmpty())
                icons.append(QPair<QUrl, QAction*>(baseUrl.resolved(image), action));
        }
        item = item.nextSiblingElement("menu");
    }

    /* add static entries */
    menu->addSeparator();

    action = menu->addAction(QIcon(":/chat.png"), tr("&Chat"));
    connect(action, SIGNAL(triggered(bool)), chat, SLOT(show()));

    action = menu->addAction(QIcon(":/diagnostics.png"), tr("&Diagnostics"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showDiagnostics()));

    action = menu->addAction(QIcon(":/photos.png"), tr("Upload &photos"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showPhotos()));

    action = menu->addAction(QIcon(":/remove.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    setContextMenu(menu);

    /* fetch icons */
    fetchIcon();

    /* connect to chat */
    QAuthenticator auth;
    Wallet::instance()->onAuthenticationRequired(baseUrl.host(), &auth);
    chat->open(auth.user(), auth.password());

}

void TrayIcon::showPhotos()
{
    if (!photos)
    {
        QAction *action = qobject_cast<QAction *>(sender());
        photos = new Photos("wifirst://www.wifirst.net/w");
        photos->setSystemTrayIcon(this);
    }
    photos->show();
    photos->raise();
}

UpdatesDialog::UpdatesDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    /* progress */
    statusLabel = new QLabel;
    layout->addWidget(statusLabel);
    progressBar = new QProgressBar;
    layout->addWidget(progressBar);
    setLayout(layout);

    /* check for updates */
    updates = new Updates(this);
    connect(updates, SIGNAL(updateAvailable(const Release&)), this, SLOT(updateAvailable(const Release&)));
    connect(updates, SIGNAL(updateDownloaded(const QUrl&)), this, SLOT(updateDownloaded(const QUrl&)));
    connect(updates, SIGNAL(updateProgress(qint64, qint64)), this, SLOT(updateProgress(qint64, qint64)));
    updates->check(QString::fromLatin1(WDESKTOP_UPDATES),
        QString::fromLatin1(WDESKTOP_VERSION));
}

void UpdatesDialog::updateAvailable(const Release &release)
{
    if (QMessageBox::question(NULL,
        tr("Update available"),
        tr("Version %1 of %2 is available. Do you want to install it")
            .arg(release.version)
            .arg(release.package),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        QString downloadDir = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
        statusLabel->setText(tr("Downloading"));
        show();
        updates->download(release, downloadDir);
    }
}

void UpdatesDialog::updateDownloaded(const QUrl &url)
{
    QDesktopServices::openUrl(url);
    qApp->quit();
}

void UpdatesDialog::updateProgress(qint64 done, qint64 total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

