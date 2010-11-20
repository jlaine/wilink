#include <signal.h>
#include <cstdlib>

#include <QCoreApplication>
#include <QSettings>

#include "QXmppLogger.h"

#include "../plugins/phone/sip.h"

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
    m_phoneNumber = settings.value("phoneNumber").toString();

    connect(m_client, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            QXmppLogger::getLogger(), SLOT(log(QXmppLogger::MessageType,QString)));
    connect(m_client, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_client, SIGNAL(disconnected()), this, SIGNAL(finished()));
}

void PhoneTester::connected()
{
    if (m_phoneNumber.isEmpty())
        return;
    QString recipient = "sip:" + m_phoneNumber;
    if (!recipient.contains("@"))
        recipient += "@" + m_client->serverName();
    SipCall *call = m_client->call(recipient);
}

void PhoneTester::start()
{
    m_client->connectToServer();
}

void PhoneTester::stop()
{
    m_client->disconnectFromServer();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion("1.0.0");

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

