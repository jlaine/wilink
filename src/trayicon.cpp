/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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
#include <QDialog>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QMenu>
#include <QSettings>
#include <QTimer>

#include "qnetio/wallet.h"

#include "chat.h"
#include "chat_accounts.h"
#include "application.h"
#include "trayicon.h"
#include "updatesdialog.h"

using namespace QNetIO;

static const QUrl baseUrl("https://www.wifirst.net/w/");
static const QString authSuffix = "@wifirst.net";
static int retryInterval = 15000;

/** Returns the authentication realm for the given JID.
 */
static QString authRealm(const QString &jid)
{
    QString domain = jid.split("@").last();
    if (domain == "wifirst.net")
        return "www.wifirst.net";
    else if (domain == "gmail.com")
        return "www.google.com";
    return domain;
}

TrayIcon::TrayIcon()
{
    /* initialise settings */
    settings = new QSettings(this);

    /* set icon */
#ifdef Q_WS_MAC
    setIcon(QIcon(":/wiLink-black.png"));
#else
    setIcon(QIcon(":/wiLink.png"));
#endif

#ifndef Q_WS_MAC
    /* catch left clicks, except on OS X */
    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
#endif

    /* prepare network manager */
    connect(Wallet::instance(), SIGNAL(credentialsRequired(const QString&, QAuthenticator *)), this, SLOT(getCredentials(const QString&, QAuthenticator *)));

    /* check for updates */
    updates = new UpdatesDialog;

    /* show chat windows */
    QTimer::singleShot(0, this, SLOT(resetChats()));
}

TrayIcon::~TrayIcon()
{
    foreach (Chat *chat, chats)
        delete chat;
    delete updates;
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
    QLineEdit *usernameEdit = new QLineEdit();
    layout->addWidget(usernameEdit, 1, 1);

    layout->addWidget(new QLabel("Password:"), 2, 0);
    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit, 2, 1);

    QPushButton *btn = new QPushButton("OK");
    dialog->connect(btn, SIGNAL(clicked()), dialog, SLOT(accept()));
    layout->addWidget(btn, 3, 1);

    dialog->setLayout(layout);

    /* prompt user */
    QString username, password;
    username = authenticator->user();
    while (username.isEmpty() || password.isEmpty())
    {
        usernameEdit->setText(username);
        passwordEdit->setText(password);
        if (!dialog->exec())
            return;
        username = usernameEdit->text().trimmed().toLower();
        password = passwordEdit->text().trimmed();
    }
    if (realm == baseUrl.host() && !username.endsWith(authSuffix))
        username += authSuffix;
    authenticator->setUser(username);
    authenticator->setPassword(password);

    /* store credentials */
    QNetIO::Wallet::instance()->setCredentials(realm, username, password);
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        showChats();
#if 0
        // FIXME: this implies that the system tray is at the bottom of the screen..
        QMenu *menu = contextMenu();
        QPoint delta = QPoint(0, menu->sizeHint().height());
        menu->popup(geometry().topLeft() - delta);
#endif
    }
}

void TrayIcon::resetChats()
{
    /* close any existing chats */
    foreach (Chat *chat, chats)
        delete chat;
    chats.clear();

    /* get chat accounts */
    QString baseJid;
    QAuthenticator auth;
    Wallet::instance()->onAuthenticationRequired(baseUrl.host(), &auth);
    baseJid = auth.user();

    QStringList chatJids;
    chatJids.append(baseJid);
    chatJids += settings->value("ChatAccounts").toStringList();

    /* connect to chat accounts */
    int xpos = 30;
    int ypos = 20;
    foreach (const QString &jid, chatJids)
    {
        QAuthenticator auth;
        auth.setUser(jid);
        Wallet::instance()->onAuthenticationRequired(authRealm(jid), &auth);

        Chat *chat = new Chat(this);
        connect(chat, SIGNAL(showAccounts()), this, SLOT(showAccounts()));
        chat->move(xpos, ypos);
        if (chatJids.size() == 1)
            chat->setWindowTitle(qApp->applicationName());
        else
            chat->setWindowTitle(QString("%1 - %2").arg(auth.user(), qApp->applicationName()));
        chat->show();

        QString domain = jid.split("@").last();
        bool ignoreSslErrors = domain != "wifirst.net";
        chat->open(auth.user(), auth.password(), ignoreSslErrors);
        chats << chat;
        xpos += 300;
    }

    /* show chats */
    showChats();
}

void TrayIcon::showAccounts()
{
    ChatAccounts dlg;

    QAuthenticator auth;
    Wallet::instance()->onAuthenticationRequired(baseUrl.host(), &auth);
    QString baseAccount = auth.user();

    QStringList accounts = settings->value("ChatAccounts").toStringList();
    accounts.prepend(baseAccount);
    dlg.setAccounts(accounts);
    if (dlg.exec() && dlg.accounts() != accounts)
    {
        QStringList newAccounts = dlg.accounts();

        // clean credentials
        foreach (const QString &account, accounts)
        {
            if (!newAccounts.contains(account))
            {
                const QString realm = authRealm(account);
                qDebug() << "Removing credentials for" << realm;
                Wallet::instance()->deleteCredentials(realm);
            }
        }

        // store new settings
        accounts.clear();
        foreach (const QString &account, newAccounts)
            if (!account.endsWith(authSuffix))
                accounts << account;
        settings->setValue("ChatAccounts", accounts);

        // reset chats
        resetChats();
    }
}

void TrayIcon::showChats()
{
    foreach (Chat *chat, chats)
    {
        chat->show();
        chat->raise();
    }
}

