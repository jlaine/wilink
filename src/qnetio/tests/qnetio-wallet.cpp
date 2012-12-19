/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <cstdlib>
#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

#include "qnetio-wallet.h"
#include "wallet.h"

QTextStream cout(stdout), cin(stdin), cerr(stderr);

using namespace QNetIO;

const QString TEST_REALM("test.qnetio.org");
const QString TEST_USERNAME("foo-user");
const QString TEST_PASSWORD("bar-password");

inline QString promptString(const QString &prompt)
{
    QString value;
    cout << prompt;
    cout.flush();
    cin >> value;
    return value;
}

void usage()
{
    QStringList backends;
    foreach (const Wallet::Backend &backend, Wallet::backends())
        backends << backend.key;

    cout << "Usage: qnetio-wallet [options]" << endl << endl;
    cout << "Options" << endl;
    cout << " -b<backend>  use the <backend> wallet backend" << endl;
    cout << "              available: " << backends.join(", ") << endl;
    cout << " -d           delete credentials" << endl;
    cout << " -h           display help message" << endl;
    cout << " -l           list all credentials" << endl;
    cout << " -p<path>     add <path> to the plugin search path" << endl;
    cout << " -r<realm>    use <realm>" << endl;
    cout << " -s           show passwords when listing credentials" << endl;
    cout << " -u<user>     use <user>" << endl;
    cout << " -w           write user for credentials" << endl;
    cout << " -t           run tests" << endl;
}

void WalletTest::initTestCase()
{
    wallet = Wallet::instance();
    wallet->deleteCredentials(TEST_REALM, TEST_USERNAME);
}

void WalletTest::testNotFound()
{
    QString username, password;
    QVERIFY(wallet->getCredentials(TEST_REALM, username, password) == false);
}

void WalletTest::testSetCredentials()
{
    QString username, password;
    QVERIFY(wallet->setCredentials(TEST_REALM, TEST_USERNAME, TEST_PASSWORD) == true);

    /* check the credentials are OK */
    QVERIFY(wallet->getCredentials(TEST_REALM, username, password) == true);
    QCOMPARE(username, TEST_USERNAME);
    QCOMPARE(password, TEST_PASSWORD);

    /* delete the credentials */
    QVERIFY(wallet->deleteCredentials(TEST_REALM, TEST_USERNAME) == true);
    QVERIFY(wallet->getCredentials(TEST_REALM, username, password) == false);
}


int main(int argc, char *argv[])
{
    QString backend;
    QString username;
    QString realm;
    QStringList pluginPaths;
    bool showPasswords = false;
    int c;
    enum {
        OP_NONE,
        OP_HELP,
        OP_DELETE,
        OP_LIST,
        OP_READ,
        OP_TEST,
        OP_WRITE,
    } op = OP_READ;

    /* parse command line arguments */
    while ((c = getopt(argc, argv, "b:dhlp:r:u:wst")) != -1)
    {
        if (c == 'h')
        {
            op = OP_HELP;
        } else if (c == 'b') {
            backend = QString::fromLocal8Bit(optarg);
        } else if (c == 'd') {
            op = OP_DELETE;
        } else if (c == 'l') {
            op = OP_LIST;
        } else if (c == 'p') {
            pluginPaths << QString::fromLocal8Bit(optarg);
        } else if (c == 'r') {
            realm = QString::fromLocal8Bit(optarg);
        } else if (c == 's') {
            showPasswords = true;
        } else if (c == 'u') {
            username = QString::fromLocal8Bit(optarg);
        } else if (c == 'w') {
            op = OP_WRITE;
        } else if (c == 't') {
            op = OP_TEST;
        }
    }

    /* create application */
    QCoreApplication app(argc, argv);
    app.setApplicationName("QNetIO");
    if (!pluginPaths.isEmpty())
        app.setLibraryPaths(app.libraryPaths() + pluginPaths);

    if (op == OP_HELP)
    {
        usage();
        return EXIT_SUCCESS;
    }

    /* create wallet */
    Wallet *wallet = Wallet::create(backend);
    if (!wallet)
    {
        qDebug("No wallet was found to run the tests");
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    switch (op)
    {
    case OP_DELETE:
        {
            if (realm.isEmpty()) {
                qWarning("No realm specified");
                ret = EXIT_FAILURE;
            } else {
                qDebug() << "Deleting credentials for" << realm;
                if (wallet->deleteCredentials(realm, username))
                    ret = EXIT_SUCCESS;
                else {
                    qWarning() << "Could not delete credentials for" << realm;
                    ret = EXIT_FAILURE;
                }
            }
        }
        break;

    case OP_LIST:
        {
            QStringList realms;
            qDebug() << "Listing all credentials";
            if (!wallet->getRealms(realms))
            {
                qWarning() << "Could not get realms"; 
                ret = EXIT_FAILURE;
                break;
            }
            
            ret = EXIT_SUCCESS;
            foreach(const QString &realm, realms)
            {
                QString username, password;
                if (wallet->getCredentials(realm, username, password))
                {
                    if (!showPasswords)
                        password = QString(password.size(), '*');
                    cout << "-- Realm : " << realm << " --" << endl;
                    cout << "Username : " << username << endl;
                    cout << "Password : " << password << endl;
                } else {
                    qWarning() << "Could not get credentials for" << realm;
                    ret = EXIT_FAILURE;
                    break;
                }
            }
        }
        break;

    case OP_READ:
        {
            if (realm.isEmpty()) {
                qWarning("No realm specified");
                ret = EXIT_FAILURE;
            } else {
                QString password;
                qDebug() << "Reading credentials for" << realm;
                if (wallet->getCredentials(realm, username, password))
                {
                    if (!showPasswords)
                        password = QString(password.size(), '*');
                    cout << "Username : " << username << endl;
                    cout << "Password : " << password << endl;
                    ret = EXIT_SUCCESS;
                } else {
                    qWarning() << "Could not find credentials for" << realm;
                    ret = EXIT_FAILURE;
                }
            }
        }
        break;

    case OP_WRITE:
        {
            if (realm.isEmpty()) {
                qWarning("No realm specified");
                ret = EXIT_FAILURE;
            } else {
                qDebug() << "Enter credentials for" << realm;
                if (username.isEmpty())
                    username = promptString("Username : ");
#ifdef WIN32
                QString password = promptString("Password : ");
#else
                QString password = QString::fromLocal8Bit(getpass("Password : "));
#endif
                if (wallet->setCredentials(realm, username, password))
                    ret = EXIT_SUCCESS;
                else {
                    qWarning() << "Could not write credentials for" << realm;
                    ret = EXIT_FAILURE;
                }
            }
        }
        break;

    case OP_TEST:
        {
            WalletTest test;
            QTest::qExec(&test);
        }
        break;

    default:
        qWarning("No operation specified");
        usage();
    }

    return ret;
}

