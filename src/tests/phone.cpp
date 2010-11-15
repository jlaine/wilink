#include <signal.h>
#include <cstdlib>

#include <QCoreApplication>

#include "QXmppLogger.h"
#include "../plugins/phone/sip.h"

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
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
    SipClient client;
    client.connectToServer();

    int ret = app.exec();
    client.disconnectFromServer();

    return ret;
}

