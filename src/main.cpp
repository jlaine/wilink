/*
 * boc-utils
 * Copyright (C) 2008-2009 Bolloré telecom
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

#include <signal.h>

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>

#include "qnetio/wallet.h"
#include "trayicon.h"

/** A AuthPrompt is a dialog to prompt the user for credentials.
 */
class AuthPrompt : public QDialog
{
public:
    AuthPrompt(const QString &prompt, QWidget *parent = NULL);

    QLineEdit *user;
    QLineEdit *password;
};

AuthPrompt::AuthPrompt(const QString &prompt, QWidget *parent)
    : QDialog(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    setLayout(layout);

    layout->addWidget(new QLabel(prompt), 0, 0, 1, 2);

    layout->addWidget(new QLabel("User:"), 1, 0);
    user = new QLineEdit();
    layout->addWidget(user, 1, 1);

    layout->addWidget(new QLabel("Password:"), 2, 0);
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    layout->addWidget(password, 2, 1);

    QPushButton *btn = new QPushButton("OK");
    connect(btn, SIGNAL(clicked()), this, SLOT(accept()));
    layout->addWidget(btn, 3, 1);
}

static void signal_handler(int sig)
{
    qApp->quit();
}

int main(int argc, char *argv[])
{
    const QString authRealm = "wifirst.net";
    const QString authSuffix = "@wifirst.net";

    /* Create application */
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Prepare wallet */
    QString user, password;
    QNetIO::Wallet *wallet = QNetIO::Wallet::instance();
    if (wallet && !wallet->getCredentials(authRealm, user, password))
    {
        AuthPrompt prompt("Please enter your credentials.");
        while (user.isEmpty() || password.isEmpty())
        {
            if (!prompt.exec())
                return EXIT_FAILURE;
            user = prompt.user->text();
            password = prompt.password->text();
        }
        if (!user.endsWith(authSuffix))
            user += authSuffix;
        wallet->setCredentials(authRealm, user, password);
    }

    /* Setup system tray icon */
    TrayIcon trayIcon;

    /* Run application */
    return app.exec();
}

