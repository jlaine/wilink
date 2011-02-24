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

#include <signal.h>
#include <cstdlib>

#include <QCoreApplication>
#include <QSettings>
#include <QTime>
#include <QTimer>

#include "QXmppLogger.h"

#include "../plugins/phone/sip.h"

#include "config.h"
#include "phone.h"

static PhoneTester tester;
static int aborted = 0;

static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    tester.stop();
    aborted = 1;
}

PhoneTester::PhoneTester(QObject *parent)
    : QObject(parent)
{
    QSettings settings("sip.conf", QSettings::IniFormat);

    m_client = new SipClient(this);
    m_client->setDomain(settings.value("domain").toString());
    m_client->setDisplayName(settings.value("displayName").toString());
    m_client->setUsername(settings.value("username").toString());
    m_client->setPassword(settings.value("password").toString());
    QString phoneNumber = settings.value("phoneNumber").toString();
    if (!phoneNumber.isEmpty())
        m_phoneQueue << phoneNumber;

    connect(m_client, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            QXmppLogger::getLogger(), SLOT(log(QXmppLogger::MessageType,QString)));
    connect(m_client, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_client, SIGNAL(disconnected()), this, SIGNAL(finished()));
    connect(m_client, SIGNAL(callReceived(SipCall*)), this, SLOT(received(SipCall*)));
}

void PhoneTester::connected()
{
    if (m_phoneQueue.isEmpty())
        return;
    QString recipient = "sip:" + m_phoneQueue.takeFirst();
    if (!recipient.contains("@"))
        recipient += "@" + m_client->domain();
    m_client->call(QString("<%1>").arg(recipient));
}

void PhoneTester::received(SipCall *call)
{
    QTimer::singleShot(3000, call, SLOT(accept()));
}

void PhoneTester::start()
{
    m_client->connectToServer();
}

void PhoneTester::stop()
{
    if (m_client->state() == SipClient::ConnectedState)
        m_client->disconnectFromServer();
    else
        emit finished();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion(WILINK_VERSION);

    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::StdoutLogging);
    QObject::connect(&tester, SIGNAL(finished()), qApp, SLOT(quit()));
    tester.start();
    return app.exec();
}

