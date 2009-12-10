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

#include <QHostInfo>
#include <QLayout>
#include <QNetworkInterface>
#include <QPushButton>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QTextEdit>
#include <QThread>

#include "diagnostics.h"
#include "wireless.h"

#define SERVER_ADDRESS QHostAddress("213.91.4.201")

static QString interfaceName(const QNetworkInterface &interface)
{
#if QT_VERSION >= 0x040500
    return interface.humanReadableName();
#else
    return interface.name();
#endif
}

static QString osName()
{
#ifdef Q_OS_LINUX
    return QString::fromLatin1("Linux");
#endif
#ifdef Q_OS_MAC
    return QString::fromLatin1("Mac OS");
#endif
#ifdef Q_OS_WIN
    return QString::fromLatin1("Windows");
#endif
    return QString::fromLatin1("Unknown");
}

static QString osVersion()
{
#ifdef Q_OS_LINUX
    QProcess process;
    process.start(QString("uname"), QStringList(QString("-r")), QIODevice::ReadOnly);
    process.waitForFinished();
    return QString::fromLocal8Bit(process.readAllStandardOutput());
#endif
#ifdef Q_OS_MAC
    switch (QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_TIGER:
        return QString::fromLatin1("Tiger");
    case QSysInfo::MV_LEOPARD:
        return QString::fromLatin1("Leopard");
    case QSysInfo::MV_SNOWLEOPARD:
        return QString::fromLatin1("Snow Leopard");
    }
#endif
#ifdef Q_OS_WIN
    switch (QSysInfo::WindowsVersion)
    {
    case QSysInfo::WV_XP:
        return QString::fromLatin1("XP");
    case QSysInfo::WV_2003:
        return QString::fromLatin1("Server 2003");
    case QSysInfo::WV_VISTA:
        return QString::fromLatin1("Vista");
    case QSysInfo::WV_WINDOWS7:
        return QString::fromLatin1("7");
    }
#endif
    return QString();
}

class Ping
{
public:
    Ping();
    Ping(const QHostAddress &host, int maxPackets = 1);

    QHostAddress hostAddress;

    // in milliseconds
    float minimumTime;
    float maximumTime;
    float averageTime;

    int sentPackets;
    int receivedPackets;
};

Ping::Ping()
    : minimumTime(0.0), maximumTime(0.0), averageTime(0.0),
    sentPackets(0), receivedPackets(0)
{
}

class NetworkInfo
{
public:
    static Ping ping(const QHostAddress &host, int maxPackets = 1);
    static QList<Ping> traceroute(const QHostAddress &host, int maxPackets = 1, int maxHops = 0);
};

Ping NetworkInfo::ping(const QHostAddress &host, int maxPackets)
{
    Ping info;
    info.hostAddress = host;

    QString program;
    if (host.protocol() == QAbstractSocket::IPv4Protocol)
    {
        program = "ping";
    } else {
        program = "ping6";
    }

    QStringList arguments;
#ifdef Q_OS_WIN
    arguments << "-n" << QString::number(maxPackets);
#else
    arguments << "-c" << QString::number(maxPackets);
#endif
    arguments << host.toString();

    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    process.waitForFinished(60000);

    /* process stats */
    QString result = QString::fromLocal8Bit(process.readAllStandardOutput());

#ifdef Q_OS_WIN
    /* min max avg */
    QRegExp timeRegex(" = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms");
    if (timeRegex.indexIn(result))
    {
        info.minimumTime = timeRegex.cap(1).toInt();
        info.maximumTime = timeRegex.cap(2).toInt();
        info.averageTime = timeRegex.cap(3).toInt();
    }
    QRegExp packetRegex(" = ([0-9]+), [^ ]+ = ([0-9]+),");
#else
    /* min/avg/max/stddev */
    QRegExp timeRegex(" = ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms");
    if (timeRegex.indexIn(result))
    {
        info.minimumTime = timeRegex.cap(1).toFloat();
        info.averageTime = timeRegex.cap(2).toFloat();
        info.maximumTime = timeRegex.cap(3).toFloat();
    }
    /* Linux  : 2 packets transmitted, 2 received, 0% packet loss, time 1001ms */
    /* Mac OS : 2 packets transmitted, 1 packets received, 50.0% packet loss */
    QRegExp packetRegex("([0-9]+) [^ ]+ [^ ]+, ([0-9]+) [^ ]+( [^ ]+)?, [0-9]+");
#endif
    if (packetRegex.indexIn(result))
    {
        info.sentPackets = packetRegex.cap(1).toInt();
        info.receivedPackets = packetRegex.cap(2).toInt();
    }
    return info;
}

QList<Ping> NetworkInfo::traceroute(const QHostAddress &host, int maxPackets, int maxHops)
{
    QList<Ping> hops;
    QString program;
    QStringList arguments;

    if (host.protocol() != QAbstractSocket::IPv4Protocol)
    {
        qWarning("IPv6 traceroute is not supported");
        return hops;
    } else {
#ifdef Q_OS_WIN
        program = "tracert";
#else
        program = "traceroute";
#endif
    }

#ifdef Q_OS_WIN
    if (maxPackets > 0)
        arguments << "-h" << QString::number(maxHops);
#else
    arguments << "-n";
    if (maxPackets > 0)
        arguments << "-q" << QString::number(maxPackets);
    if (maxHops > 0)
        arguments << "-m" << QString::number(maxHops);
#endif
    arguments << host.toString();

    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    process.waitForFinished(60000);

    /* process results */
    QString result = QString::fromLocal8Bit(process.readAllStandardOutput());

    /*
     * Windows :  1  6.839 ms  19.866 ms  1.134 ms 192.168.99.1
     * Windows :  2  <1 ms  <1 ms  <1 ms 192.168.99.1
     * *nix    :  1  192.168.99.1  6.839 ms  19.866 ms  1.134 ms
     *            2  192.168.99.1  6.839 ms * 1.134 ms
     */
    QRegExp hopRegex("\\s+[0-9]+\\s+(.+)");
    QRegExp ipRegex("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
    QRegExp timeRegex("([0-9.]+|<1)");
    foreach (const QString &line, result.split("\n"))
    {
        if (hopRegex.exactMatch(line))
        {
            Ping hop;
            float totalTime = 0.0;

            foreach (const QString &bit, hopRegex.cap(1).split(QRegExp("\\s+")))
            {
                if (ipRegex.exactMatch(bit))
                    hop.hostAddress = QHostAddress(bit);
                else if (bit == "*")
                    hop.sentPackets++;
                else if (timeRegex.exactMatch(bit))
                {
                    float hopTime = (bit == "<1") ? 0 : bit.toFloat();
                    hop.sentPackets += 1;
                    hop.receivedPackets += 1;
                    if (hopTime > hop.maximumTime)
                        hop.maximumTime = hopTime;
                    if (!hop.minimumTime || hopTime < hop.minimumTime)
                        hop.minimumTime = hopTime;
                    totalTime += hopTime;
                }
            }
            if (hop.receivedPackets)
                hop.averageTime = totalTime / static_cast<float>(hop.receivedPackets);
            hops.append(hop);
        }
    }
    return hops;
}

static QString dumpPings(const QList<Ping> &pings)
{
    QString info = "<table>";
    info += "<tr><th>Host</th><th>Packets received</th><th>Times</th></tr>";
    foreach (const Ping &report, pings)
    {
        info += QString("<tr style=\"background-color: %1\"><td>%2</td><td align=\"center\">%3 / %4</td><td>%5</td></tr>")
            .arg(report.receivedPackets == report.sentPackets ? "green" : "red")
            .arg(report.hostAddress.toString())
            .arg(report.receivedPackets)
            .arg(report.sentPackets)
            .arg(report.receivedPackets == 0 ? "unreachable" : QString("min: %1 ms, max: %2 ms, avg: %3 ms")
                .arg(report.minimumTime)
                .arg(report.maximumTime)
                .arg(report.averageTime));
    }
    info += "</table>";
    return info;
}

/* NETWORK */

class NetworkThread : public QThread
{
public:
    NetworkThread(QObject *parent) : QThread(parent) {};
    void run();

    QList<QHostInfo> lookups;
    QList<Ping> pings;
    QList<Ping> traceroute;
};

void NetworkThread::run()
{
    QList<QHostAddress> gateways;

    /* try to detemine gateways */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                gateways.append(QHostAddress((entry.ip().toIPv4Address() & entry.netmask().toIPv4Address()) + 1));
                break;
            }
        }
    }
    gateways.append(SERVER_ADDRESS);

    /* run DNS tests */
    QList<QHostAddress> longPing;
    QStringList hostNames;
    hostNames << "wireless.wifirst.fr" << "www.wifirst.net" << "www.google.fr";
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

    /* run ping tests */
    foreach (const QHostAddress &gateway, gateways)
        pings.append(NetworkInfo::ping(gateway, longPing.contains(gateway) ? 30 : 3));

    /* run traceroute */
    traceroute = NetworkInfo::traceroute(SERVER_ADDRESS, 3, 4);
}

/* WIRELESS */

class WirelessResult
{
public:
    QNetworkInterface interface;
    QList<WirelessNetwork> availableNetworks;
    WirelessNetwork currentNetwork;
};

class WirelessThread : public QThread
{
public:
    WirelessThread(QObject *parent) : QThread(parent) {};
    void run();

    QList<WirelessResult> results;
};

void WirelessThread::run()
{
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        WirelessInterface wireless(interface);
        if (!wireless.isValid())
            continue;

        WirelessResult result;
        result.interface = interface;
        result.availableNetworks = wireless.availableNetworks();
        result.currentNetwork = wireless.currentNetwork();
        results.append(result);
    }
}

/* GUI */

Diagnostics::Diagnostics(QWidget *parent)
    : QDialog(parent), networkThread(NULL), wirelessThread(NULL)
{
    QVBoxLayout *layout = new QVBoxLayout;

    text = new QTextEdit;
    text->setReadOnly(true);
    layout->addWidget(text);

    QHBoxLayout *hbox = new QHBoxLayout;
    refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    hbox->addWidget(refreshButton);

    hbox->addStretch();

    printButton = new QPushButton(tr("Print"));
    connect(printButton, SIGNAL(clicked()), this, SLOT(print()));
    hbox->addWidget(printButton);

    layout->addItem(hbox);
    setLayout(layout);
    setMinimumSize(QSize(450, 400));

    refresh();
}

void Diagnostics::print()
{
    QPrinter printer;
    QPrintDialog dialog(&printer);
    if (dialog.exec() == QDialog::Accepted)
        text->print(&printer);
}

void Diagnostics::refresh()
{
    refreshButton->setEnabled(false);
    printButton->setEnabled(false);

    /* show system info */
    text->setText("<h1>Diagnostics for " + osName() + " " + osVersion() + "</h1>");
    text->append("<h2>Network interfaces</h2>");
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        QString info = "<h3>Interface " + interfaceName(interface) + "</h3>";
        if (interface.addressEntries().size())
        {
            info += "<ul>";
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
            {
                QString protocol;
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                    protocol = "IPv4";
                else if (entry.ip().protocol() == QAbstractSocket::IPv6Protocol)
                    protocol = "IPv6";
                else
                    continue;
                info += "<li>" + protocol + " address: " + entry.ip().toString() + "</li>";
                info += "<li>" + protocol + " netmask: " + entry.netmask().toString() + "</li>";
            }
            info += "</ul>";
        } else {
            info += "<p>No address</p>";
        }
        text->append(info);
    }

    /* run network tests */
    networkThread = new NetworkThread(this);
    connect(networkThread, SIGNAL(finished()), this, SLOT(networkFinished()));
    networkThread->start();
}

void Diagnostics::networkFinished()
{
    QString info = "<h2>Tests</h2>";

    /* DNS tests */
    info += "<h3>DNS</h3>";
    info += "<table>";
    info += "<tr><th>Host name</th><th>Host address</th></tr>";
    foreach (const QHostInfo &hostInfo, networkThread->lookups)
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
        info += QString("<tr style=\"background-color: %1\"><td>%2</td><td>%3</td></tr>")
            .arg(hostInfo.error() == QHostInfo::NoError ? "green" : "red")
            .arg(hostInfo.hostName())
            .arg(hostAddress);
    }
    info += "</table>";
    text->append(info);

    /* ping tests */
    text->append("<h3>Ping</h3>" + dumpPings(networkThread->pings));

    /* traceroute tests */
    text->append("<h3>Traceroute</h3>" + dumpPings(networkThread->traceroute));

    /* get wireless info */
    wirelessThread = new WirelessThread(this);
    connect(wirelessThread, SIGNAL(finished()), this, SLOT(wirelessFinished()));
    wirelessThread->start();
}

void Diagnostics::wirelessFinished()
{
    QString info;
    foreach (const WirelessResult &result, wirelessThread->results)
    {
        info += "<h2>Wireless interface " + interfaceName(result.interface) + "</h2>";

        info += "<h3>Current network</h3>";
        if (result.currentNetwork.isValid())
        {
            info += "<ul>";
            info += "<li>SSID: "+ result.currentNetwork.ssid() + "</li>";
            info += "<li>RSSI: " + QString::number(result.currentNetwork.rssi()) + "</li>";
            if (result.currentNetwork.cinr())
                info += "<li>CINR: " + QString::number(result.currentNetwork.cinr()) + "</li>";
            info += "</ul>";
        }

        info += "<h3>Available networks</h3>";
        info += "<table>";
        info += "<tr><th>SSID</th><th>RSSI</th><th>CINR</th></tr>";
        foreach (const WirelessNetwork &network, result.availableNetworks)
        {
            if (network != result.currentNetwork)
            {
                info += "<tr>";
                info += "<td>" + network.ssid() + "</td>";
                info += "<td>" + QString::number(network.rssi()) + "</td>";
                info += "<td>" + (network.cinr() ? QString::number(network.cinr()) : QString()) + "</td>";
                info += "</tr>";
            }
        }
        info += "</table>";
    }
    text->append(info);

    /* enable buttons */
    refreshButton->setEnabled(true);
    printButton->setEnabled(true);
}

