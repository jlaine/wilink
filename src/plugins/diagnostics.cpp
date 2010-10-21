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

#include <QCoreApplication>
#include <QDateTime>
#include <QDomElement>
#include <QLineEdit>
#include <QLayout>
#include <QMenu>
#include <QNetworkInterface>
#include <QPushButton>
#include <QProgressBar>
#include <QShortcut>
#include <QTextBrowser>
#include <QTimer>
#include <QThread>

#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "systeminfo.h"

#include "diagnostics.h"

#define DIAGNOSTICS_ROSTER_ID "0_diagnostics"

static QThread *diagnosticsThread = 0;

static const QHostAddress serverAddress("213.91.4.201");

Q_DECLARE_METATYPE(QList<QHostInfo>)
Q_DECLARE_METATYPE(QList<Ping>)
Q_DECLARE_METATYPE(QList<Traceroute>)
Q_DECLARE_METATYPE(Interface)

static int id1 = qRegisterMetaType< QList<QHostInfo> >();
static int id2 = qRegisterMetaType< QList<Ping> >();
static int id3 = qRegisterMetaType< QList<Traceroute> >();
static int id4 = qRegisterMetaType< Interface >();

/* NETWORK */

void DiagnosticsAgent::lookup(const DiagnosticsIq &request, QObject *receiver, const char *member)
{
    if (!diagnosticsThread)
    {
        diagnosticsThread = new QThread;
        diagnosticsThread->start();
    }

    DiagnosticsAgent *agent = new DiagnosticsAgent;
    agent->moveToThread(diagnosticsThread);
    connect(agent, SIGNAL(results(DiagnosticsIq)), receiver, member);
    QMetaObject::invokeMethod(agent, "handle", Qt::QueuedConnection,
        Q_ARG(DiagnosticsIq, request));
}

void DiagnosticsAgent::handle(const DiagnosticsIq &request)
{
    DiagnosticsIq iq;
    iq.setId(request.id());
    iq.setFrom(request.to());
    iq.setTo(request.from());
    iq.setType(QXmppIq::Result);

    QList<QHostAddress> gateways;
    QList<QHostInfo> lookups;
    int done = 0;
    int total = 3;

    emit progress(done, total);

    /* software versions */
    QList<Software> softwares;
    Software os;
    os.setType("os");
    os.setName(SystemInfo::osName());
    os.setVersion(SystemInfo::osVersion());
    softwares << os;

    Software app;
    app.setType("application");
    app.setName(qApp->applicationName());
    app.setVersion(qApp->applicationVersion());
    softwares << app;
    iq.setSoftwares(softwares);

    /* discover interfaces */
    QList<Interface> interfaceResults;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) ||
            (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        Interface result;
#if QT_VERSION >= 0x040500
        result.setName(interface.humanReadableName());
#else
        result.setName(interface.name());
#endif

        // addresses
        result.setAddressEntries(interface.addressEntries());

        // wireless
        WirelessInterface wireless(interface);
        if (wireless.isValid())
        {
            QList<WirelessNetwork> networks = wireless.availableNetworks();
            WirelessNetwork current = wireless.currentNetwork();
            if (current.isValid())
            {
                networks.removeAll(current);
                networks.prepend(current);
            }
            result.setWirelessNetworks(networks);
            result.setWirelessStandards(wireless.supportedStandards());
        }
        interfaceResults << result;
        emit progress(++done, ++total);
    }
    iq.setInterfaces(interfaceResults);

    /* try to detemine gateways */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.netmask().isNull() && entry.netmask() != QHostAddress::Broadcast)
            {
                gateways.append(QHostAddress((entry.ip().toIPv4Address() & entry.netmask().toIPv4Address()) + 1));
                break;
            }
        }
    }
    gateways.append(serverAddress);

    /* run DNS tests */
    QList<QHostAddress> longPing;
    QStringList hostNames;
    hostNames << "wireless.wifirst.net" << "www.wifirst.net" << "www.google.fr";
    foreach (const QString &hostName, hostNames)
    {
        QHostInfo hostInfo = QHostInfo::fromName(hostName);
        lookups.append(hostInfo);
        if (hostInfo.error() == QHostInfo::NoError)
        {
            foreach (const QHostAddress &address, hostInfo.addresses())
            {
                if (address.protocol() == QAbstractSocket::IPv4Protocol)
                {
                    if (!gateways.contains(address))
                        gateways.append(address);
                    if (hostName == "www.google.fr")
                        longPing.append(address);
                    break;
                }
            }
        }
    }
    emit progress(++done, total);
    iq.setLookups(lookups);

    /* run ping tests */
    QList<Ping> pings;
    foreach (const QHostAddress &gateway, gateways)
    {
        const Ping &ping = NetworkInfo::ping(gateway, 3);
        pings.append(ping);
        if (longPing.contains(gateway) && ping.sentPackets() != ping.receivedPackets())
            pings.append(NetworkInfo::ping(gateway, 30));
    }
    emit progress(++done, total);
    iq.setPings(pings);

    /* run traceroute */
    QList<Traceroute> traceroutes;
    traceroutes << NetworkInfo::traceroute(serverAddress, 3, 4);
    emit progress(++done, total);
    iq.setTraceroutes(traceroutes);

    emit results(iq);
}

/* GUI */

class TextList : public QStringList
{
public:
    QString render() const {
        QString info("<ul>");
        foreach (const QString &bit, *this)
            info += QString("<li>%1</li>").arg(bit);
        return info + "</ul>";
    };
};

class TextRow : public QStringList
{
public:
    TextRow(bool title=false) : rowTitle(title) {};
    QString render() const {
        QString info = rowColor.isEmpty() ? QString("<tr>") : QString("<tr style=\"background-color: %1\">").arg(rowColor);
        QString rowTemplate = rowTitle ? QString("<th>%1</th>") : QString("<td>%1</td>");
        foreach (const QString &bit, *this)
            info += rowTemplate.arg(bit);
        return info + "</tr>";
    };
    void setColor(const QString &color) {
        rowColor = color;
    };

private:
    QString rowColor;
    bool rowTitle;
};

class TextTable : public QList<TextRow>
{
public:
    QString render() const {
        QString info("<table>");
        foreach (const TextRow &row, *this)
            info += row.render();
        return info + "</table>";
    };
};

static QString dumpPings(const QList<Ping> &pings)
{
    TextTable table;
    TextRow titles(true);
    titles << "Host" << "Packets received" << "Min. time" << "Max. time" << "Average";
    table << titles;
    foreach (const Ping &report, pings)
    {
        TextRow row;
        row.setColor(report.receivedPackets() == report.sentPackets() ? "green" : "red");
        row << (report.hostAddress().isNull() ? "-" : report.hostAddress().toString());
        row << QString("%1 / %2").arg(QString::number(report.receivedPackets()), QString::number(report.sentPackets()));
        if (report.receivedPackets())
        {
            row << QString("%1 ms").arg(report.minimumTime());
            row << QString("%1 ms").arg(report.maximumTime());
            row << QString("%1 ms").arg(report.averageTime());
        } else {
            row << "-" << "-" << "-";
        }
        table << row;
    }
    return table.render();
}

Diagnostics::Diagnostics(QXmppClient *client, QWidget *parent)
    : ChatPanel(parent),
    m_client(client),
    m_displayed(false)
{
    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(headerLayout());
    text = new QTextBrowser;
    layout->addWidget(text);

    QHBoxLayout *hbox = new QHBoxLayout;

    hostEdit = new QLineEdit;
    hbox->addWidget(hostEdit);

    progressBar = new QProgressBar;
    hbox->addWidget(progressBar);

    refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setIcon(QIcon(":/refresh.png"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    hbox->addWidget(refreshButton);

    layout->addLayout(hbox);
    setLayout(layout);
    setMinimumSize(QSize(450, 400));
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Diagnostics"));

    //connect(diagnosticsAgent, SIGNAL(progress(int, int)), this, SLOT(showProgress(int, int)));
    DiagnosticsExtension *extension = m_client->findExtension<DiagnosticsExtension>();
    connect(extension, SIGNAL(diagnosticsReceived(DiagnosticsIq)),
            this, SLOT(showResults(DiagnosticsIq)));

    connect(this, SIGNAL(showPanel()), this, SLOT(slotShow()));
    connect(this, SIGNAL(hidePanel()), this, SIGNAL(unregisterPanel()));
}

Diagnostics::~Diagnostics()
{
}

void Diagnostics::addItem(const QString &title, const QString &value)
{
    text->append(QString("<h3>%1</h3>%2").arg(title, value));
}

void Diagnostics::addSection(const QString &title)
{
    text->append(QString("<h2>%1</h2>").arg(title));
}

void Diagnostics::refresh()
{
    refreshButton->setEnabled(false);
    text->setText("<h2>System information</h2>");

    if (hostEdit->text().isEmpty())
    {
        DiagnosticsIq req;
        DiagnosticsAgent::lookup(req, this, SLOT(showResults(DiagnosticsIq)));
        return;
    }

    DiagnosticsExtension *extension = m_client->findExtension<DiagnosticsExtension>();
    extension->requestDiagnostics(hostEdit->text());
}

void Diagnostics::slotShow()
{
    if (m_displayed)
        return;
    refresh();
    m_displayed = true;
}

void Diagnostics::showLookup(const QList<QHostInfo> &results)
{
    TextTable table;
    TextRow titles(true);
    titles << "Host name" << "Host address";
    table << titles;
    foreach (const QHostInfo &hostInfo, results)
    {
        QString hostAddress = "not found";
        foreach (const QHostAddress &address, hostInfo.addresses())
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                hostAddress = address.toString();
                break;
            }
        }
        TextRow row;
        row.setColor(hostInfo.error() == QHostInfo::NoError ? "green" : "red");
        row << hostInfo.hostName() << hostAddress;
        table << row;
    }
    addItem("DNS", table.render());
}

void Diagnostics::showProgress(int done, int total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

void Diagnostics::showResults(const DiagnosticsIq &iq)
{
    // show software
    TextList list;
    foreach (const Software &software, iq.softwares())
    {
        QString title;
        if (software.type() == QLatin1String("os"))
            title = QLatin1String("Operating system");
        else if (software.type() == QLatin1String("application"))
            title = QLatin1String("Application");
        else
            title = software.type();
        list << QString("%1: %2 %3").arg(title, software.name(), software.version());
    }
    addItem("Software", list.render());

    // show interfaces
    foreach (const Interface &interface, iq.interfaces())
        showInterface(interface);

    // show tests
    addSection("Tests");
    showLookup(iq.lookups());
    addItem("Ping", dumpPings(iq.pings()));
    foreach (const Traceroute &traceroute, iq.traceroutes())
        addItem("Traceroute", dumpPings(traceroute));

    // enable buttons
    refreshButton->setEnabled(true);
#if 0
    if (m_thread)
    {
        m_thread->wait();
        delete m_thread;
        m_thread = 0;
    }
#endif
}

void Diagnostics::showInterface(const Interface &result)
{
    const bool showCinr = false;
    addSection("Network interface " + result.name());

    // addresses
    TextList list;
    foreach (const QNetworkAddressEntry &entry, result.addressEntries())
    {
        QString protocol;
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            protocol = "IPv4";
        else if (entry.ip().protocol() == QAbstractSocket::IPv6Protocol)
            protocol = "IPv6";
        else
            continue;
        list << protocol + " address: " + entry.ip().toString();
        list << protocol + " netmask: " + entry.netmask().toString();
    }
    addItem("Addresses",
        list.size() ? list.render() : "<p>No address</p>");

    // wireless
    if (result.wirelessStandards())
        addItem("Wireless standards", result.wirelessStandards().toString());

    if (!result.wirelessNetworks().isEmpty())
    {
        TextTable table;
        TextRow titles(true);
        titles << "SSID" << "RSSI";
        if (showCinr)
            titles << "CINR";
        titles << "";
        table << titles;
        foreach (const WirelessNetwork &network, result.wirelessNetworks())
        {
            TextRow row;
            row << network.ssid();
            row << QString::number(network.rssi()) + " dBm";
            if (showCinr)
                row << (network.cinr() ? QString::number(network.cinr()) : QString());
            row << (network.isCurrent() ? "<b>*</b>" : "");
            table << row;
        }
        addItem("Wireless networks", table.render());
    }
}

// EXTENSION

DiagnosticsExtension::DiagnosticsExtension(QXmppClient *client)
{
    qRegisterMetaType<DiagnosticsIq>("DiagnosticsIq");
}

QStringList DiagnosticsExtension::discoveryFeatures() const
{
    return QStringList() << ns_diagnostics;
}

void DiagnosticsExtension::handleResults(const DiagnosticsIq &results)
{
    client()->sendPacket(results);
}

bool DiagnosticsExtension::handleStanza(const QDomElement &stanza)
{
    if (stanza.tagName() == "iq" && DiagnosticsIq::isDiagnosticsIq(stanza))
    {
        DiagnosticsIq iq;
        iq.parse(stanza);

        if (iq.type() == QXmppIq::Get)
        {
            DiagnosticsAgent::lookup(iq, this, SLOT(handleResults(DiagnosticsIq)));
        } else if (iq.type() == QXmppIq::Result || iq.type() == QXmppIq::Error) {
            emit diagnosticsReceived(iq);
        }
        return true;
    }
    return false;
}

void DiagnosticsExtension::requestDiagnostics(const QString &jid)
{
    DiagnosticsIq iq;
    iq.setType(QXmppIq::Get);
    iq.setTo(jid);
    client()->sendPacket(iq);
}

// PLUGIN

class DiagnosticsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
    void finalize(Chat *chat);
};

bool DiagnosticsPlugin::initialize(Chat *chat)
{
    /* add diagnostics extension */
    const QString domain = chat->client()->configuration().domain();
    if (domain == "wifirst.net")
    {
        DiagnosticsExtension *extension = new DiagnosticsExtension(chat->client());
        chat->client()->addExtension(extension);
    }

    /* register panel */
    Diagnostics *diagnostics = new Diagnostics(chat->client());
    diagnostics->setObjectName(DIAGNOSTICS_ROSTER_ID);
    chat->addPanel(diagnostics);

    /* add menu entry */
    QList<QAction*> actions = chat->fileMenu()->actions();
    QAction *firstAction = actions.isEmpty() ? 0 : actions.first();
    QAction *action = new QAction(QIcon(":/diagnostics.png"), diagnostics->windowTitle(), chat->fileMenu());
    chat->fileMenu()->insertAction(firstAction, action);
    connect(action, SIGNAL(triggered()), diagnostics, SIGNAL(showPanel()));

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_I), chat);
    connect(shortcut, SIGNAL(activated()), diagnostics, SIGNAL(showPanel()));
    return true;
}

void DiagnosticsPlugin::finalize(Chat *chat)
{
#if 0
    if (m_thread)
    {
        m_thread->wait();
        delete m_thread;
    }
#endif
}

Q_EXPORT_STATIC_PLUGIN2(diagnostics, DiagnosticsPlugin)
