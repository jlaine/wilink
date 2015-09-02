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
#include <QDateTime>
#include <QDomElement>
#include <QNetworkInterface>
#include <QShortcut>
#include <QTimer>
#include <QThread>

#include "QXmppClient.h"
#include "QXmppUtils.h"

#include "diagnostics.h"
#include "systeminfo.h"

Q_DECLARE_METATYPE(QList<QHostInfo>)
Q_DECLARE_METATYPE(QList<Ping>)
Q_DECLARE_METATYPE(QList<Traceroute>)
Q_DECLARE_METATYPE(Interface)

static int id1 = qRegisterMetaType< QList<QHostInfo> >();
static int id2 = qRegisterMetaType< QList<Ping> >();
static int id3 = qRegisterMetaType< QList<Traceroute> >();
static int id4 = qRegisterMetaType< Interface >();

/* NETWORK */

DiagnosticsAgent::DiagnosticsAgent(const DiagnosticConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
}

void DiagnosticsAgent::handle(const QXmppDiagnosticIq &request)
{
    iq.setId(request.id());
    iq.setFrom(request.to());
    iq.setTo(request.from());
    iq.setType(QXmppIq::Result);

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

    /* get DNS servers */
    iq.setNameServers(NetworkInfo::nameServers());

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
    }
    iq.setInterfaces(interfaceResults);

    /* try to determine gateways */
    QList<QHostAddress> pingAddresses;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.netmask().isNull() && entry.netmask() != QHostAddress::Broadcast)
            {
                const QHostAddress gateway((entry.ip().toIPv4Address() & entry.netmask().toIPv4Address()) + 1);
                if (!pingAddresses.contains(gateway))
                    pingAddresses.append(gateway);
                break;
            }
        }
    }

    /* run DNS tests */
    foreach (const QString &hostName, m_config.pingHosts)
    {
        QHostAddress address;
        if (resolve(hostName, address)) {
            if (!pingAddresses.contains(address))
                pingAddresses.append(address);
        }
    }

    /* run ping tests */
    QList<Ping> pings;
    foreach (const QHostAddress &address, pingAddresses)
    {
        const Ping ping = NetworkInfo::ping(address, 3);
        pings.append(ping);
        if (address == QHostAddress("8.8.8.8") && ping.sentPackets() != ping.receivedPackets())
            pings.append(NetworkInfo::ping(address, 30));
    }
    iq.setPings(pings);

    /* run traceroute */
    QList<Traceroute> traceroutes;
    foreach (const QString &hostName, m_config.tracerouteHosts)
    {
        QHostAddress address;
        if (resolve(hostName, address)) {
            traceroutes << NetworkInfo::traceroute(address, 3, 4);
        }
    }
    iq.setTraceroutes(traceroutes);

    /* run download */
    if (m_config.transferUrl.isValid()) {
        TransferTester *runner = new TransferTester(this);
        connect(runner, SIGNAL(finished(QList<Transfer>)), this, SLOT(transfersFinished(QList<Transfer>)));
        runner->start(m_config.transferUrl);
    } else {
        emit finished(iq);
    }
}

bool DiagnosticsAgent::resolve(const QString hostName, QHostAddress &result)
{
    // check for an IP address
    if (result.setAddress(hostName)) {
        return true;
    }

    // DNS lookup
    QHostInfo hostInfo = QHostInfo::fromName(hostName);
    QList<QHostInfo> lookups = iq.lookups();
    lookups.append(hostInfo);
    iq.setLookups(lookups);
    if (hostInfo.error() == QHostInfo::NoError)
    {
        foreach (const QHostAddress &address, hostInfo.addresses())
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                result = address;
                return true;
            }
        }
    }

    // no valid address
    return false;
}

void DiagnosticsAgent::transfersFinished(const QList<Transfer> &transfers)
{
    iq.setTransfers(transfers);
    emit finished(iq);
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

static QString makeItem(const QString &title, const QString &value)
{
    return QString("<h3>%1</h3>%2").arg(title, value);
}

static QString makeSection(const QString &title)
{
    return QString("<h2>%1</h2>").arg(title);
}

static QString dumpInterface(const Interface &result)
{
    const bool showCinr = false;
    QString text = makeSection("Network interface " + result.name());

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
    text += makeItem("Addresses",
        list.size() ? list.render() : "<p>No address</p>");

    // wireless
    if (result.wirelessStandards())
        text += makeItem("Wireless standards", result.wirelessStandards().toString());

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
        text += makeItem("Wireless networks", table.render());
    }
    return text;
}

static QString dumpLookup(const QList<QHostInfo> &results)
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
    return makeItem("DNS", table.render());
}

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

static QString dumpTransfers(const QList<Transfer> &transfers)
{
    TextTable table;
    TextRow titles(true);
    titles << "Direction" << "URL" << "Time" << "Speed";
    table << titles;

    foreach (const Transfer &transfer, transfers) {
        const int speed = (transfer.time() > 0) ? (transfer.size()*1000*8)/(transfer.time()*1024) : 0;

        TextRow row;
        row.setColor(transfer.error() == 0 ? "green" : "red");
        row << (transfer.direction() == Transfer::Upload ? "upload" : "download");
        row << transfer.url().toString();
        row << QString("%1 ms").arg(transfer.time());
        row << QString("%1 kbps").arg(speed);
        table << row;
    }
    return table.render();
}

static QString dumpResults(const QXmppDiagnosticIq &iq)
{
    QString text = makeSection("System diagnostics");

    // show software
    TextList list;
    foreach (const Software &software, iq.softwares()) {
        QString title;
        if (software.type() == QLatin1String("os"))
            title = QLatin1String("Operating system");
        else if (software.type() == QLatin1String("application"))
            title = QLatin1String("Application");
        else
            title = software.type();
        list << QString("%1: %2 %3").arg(title, software.name(), software.version());
    }
    text += makeItem("Software", list.render());

    // show network info
    if (!iq.nameServers().isEmpty()) {
        TextList networkList;
        foreach (const QHostAddress &server, iq.nameServers())
            networkList << QString("DNS server: %1").arg(server.toString());
        text += makeItem("Network", networkList.render());
    }

    // show interfaces
    foreach (const Interface &interface, iq.interfaces())
        text += dumpInterface(interface);

    // show tests
    text += makeSection("Tests");
    text += dumpLookup(iq.lookups());
    if (!iq.pings().isEmpty())
        text += makeItem("Ping", dumpPings(iq.pings()));
    foreach (const Traceroute &traceroute, iq.traceroutes())
        text += makeItem(
            QString("Traceroute to %1").arg(traceroute.hostAddress().toString()),
            dumpPings(traceroute));
    if (!iq.transfers().isEmpty())
        text += makeItem("Transfer", dumpTransfers(iq.transfers()));

    return text;
}

// EXTENSION

DiagnosticManager::DiagnosticManager()
    : m_thread(0)
{
    qRegisterMetaType<QXmppDiagnosticIq>("QXmppDiagnosticIq");
}

void DiagnosticManager::setClient(QXmppClient *client)
{
    bool check;
    Q_UNUSED(check);

    QXmppClientExtension::setClient(client);

    check = connect(client, SIGNAL(diagnosticServerChanged(QString)),
                    this, SLOT(diagnosticServerChanged(QString)));
    Q_ASSERT(check);
}

void DiagnosticManager::diagnosticServerChanged(const QString &diagServer)
{
    m_diagnosticsServer = diagServer;
}

QStringList DiagnosticManager::discoveryFeatures() const
{
    return QStringList() << ns_diagnostics;
}

void DiagnosticManager::handleResults(const QXmppDiagnosticIq &results)
{
    m_html = dumpResults(results);
    emit htmlChanged(m_html);

    if (!results.to().isEmpty())
        client()->sendPacket(results);

    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
        m_thread = 0;
        emit runningChanged(false);
    }
}

bool DiagnosticManager::handleStanza(const QDomElement &stanza)
{
    if (stanza.tagName() == QLatin1String("iq") && QXmppDiagnosticIq::isDiagnosticIq(stanza)) {
        QXmppDiagnosticIq iq;
        iq.parse(stanza);

        if (iq.type() == QXmppIq::Get) {
            if (m_thread) {
                QXmppDiagnosticIq response;
                response.setType(QXmppIq::Error);
                response.setError(
                    QXmppStanza::Error(QXmppStanza::Error::Cancel, QXmppStanza::Error::ServiceUnavailable));
                response.setId(iq.id());
                response.setTo(iq.from());
                client()->sendPacket(response);
            }
            else if (iq.from() != m_diagnosticsServer) {
                QXmppDiagnosticIq response;
                response.setType(QXmppIq::Error);
                response.setError(
                    QXmppStanza::Error(QXmppStanza::Error::Cancel, QXmppStanza::Error::Forbidden));
                response.setId(iq.id());
                response.setTo(iq.from());
                client()->sendPacket(response);
            }
            else {
                run(iq);
            }
        }
        return true;
    }
    return false;
}

QString DiagnosticManager::html() const
{
    return m_html;
}

void DiagnosticManager::refresh()
{
    run(QXmppDiagnosticIq());
}

void DiagnosticManager::run(const QXmppDiagnosticIq &request)
{
    bool check;
    Q_UNUSED(check);

    if (m_thread)
        return;

    m_thread = new QThread;

    // run diagnostics
    DiagnosticsAgent *agent = new DiagnosticsAgent(m_config);
    agent->moveToThread(m_thread);
    check = connect(agent, SIGNAL(finished(QXmppDiagnosticIq)),
                    this, SLOT(handleResults(QXmppDiagnosticIq)));
    Q_ASSERT(check);

    check = connect(agent, SIGNAL(finished(QXmppDiagnosticIq)),
                    agent, SLOT(deleteLater()));
    Q_ASSERT(check);

    QMetaObject::invokeMethod(agent, "handle", Qt::QueuedConnection,
        Q_ARG(QXmppDiagnosticIq, request));

    m_thread->start();
    emit runningChanged(true);
}

bool DiagnosticManager::running() const
{
    return m_thread != 0;
}

QStringList DiagnosticManager::pingHosts() const
{
    return m_config.pingHosts;
}

void DiagnosticManager::setPingHosts(const QStringList &pingHosts)
{
    if (pingHosts != m_config.pingHosts) {
        m_config.pingHosts = pingHosts;
        emit pingHostsChanged(m_config.pingHosts);
    }
}

QStringList DiagnosticManager::tracerouteHosts() const
{
    return m_config.tracerouteHosts;
}

void DiagnosticManager::setTracerouteHosts(const QStringList &tracerouteHosts)
{
    if (tracerouteHosts != m_config.tracerouteHosts) {
        m_config.tracerouteHosts = tracerouteHosts;
        emit tracerouteHostsChanged(m_config.tracerouteHosts);
    }
}

QUrl DiagnosticManager::transferUrl() const
{
    return m_config.transferUrl;
}

void DiagnosticManager::setTransferUrl(const QUrl &transferUrl)
{
    if (transferUrl != m_config.transferUrl) {
        m_config.transferUrl = transferUrl;
        emit transferUrlChanged(m_config.transferUrl);
    }
}
