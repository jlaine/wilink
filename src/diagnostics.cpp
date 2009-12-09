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

class PingReport
{
public:
    float minRtt;
    float maxRtt;
    float averageRtt;
};

static bool ping(const QHostAddress &host, PingReport &report)
{
    if (host.protocol() == QAbstractSocket::IPv4Protocol)
    {
        QString program = "ping";
        QStringList arguments;
#ifdef Q_OS_WIN
        arguments << "-n" << "2" << host.toString();
#else
        arguments << "-c" << "2" << host.toString();
#endif

        QProcess process;
        process.start(program, arguments, QIODevice::ReadOnly);
        process.waitForFinished();

        /* process stats */
        QString result = QString::fromLocal8Bit(process.readAllStandardOutput());
        qDebug() << result;

#ifdef Q_OS_WIN
        /* min max avg */
        QRegExp regex(" = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms, [^ ]+ = ([0-9]+)ms");
        if (regex.indexIn(result))
        {
            report.minRtt = regex.cap(1).toInt();
            report.maxRtt = regex.cap(2).toInt();
            report.averageRtt = regex.cap(3).toInt();
        }
#else
        /* min/avg/max/stddev */
        QRegExp regex(" = ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms");
        if (regex.indexIn(result))
        {
            report.minRtt = regex.cap(1).toFloat();
            report.averageRtt = regex.cap(2).toFloat();
            report.maxRtt = regex.cap(3).toFloat();
        }
#endif
        return (process.exitCode() == 0);
    }
    return false;
}

/* NETWORK */

class NetworkReport
{
public:
    QHostAddress gateway_address;
    bool gateway_ping;
    PingReport gateway_report;
};

typedef QPair<QNetworkInterface, NetworkReport> NetworkResult;

class NetworkThread : public QThread
{
public:
    NetworkThread(QObject *parent) : QThread(parent) {};
    void run();
    QList<NetworkResult> results;
};

void NetworkThread::run()
{
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        NetworkReport report;
        report.gateway_ping = false;
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                report.gateway_address = QHostAddress((entry.ip().toIPv4Address() & entry.netmask().toIPv4Address()) + 1);
                report.gateway_ping = ping(report.gateway_address, report.gateway_report);
                break;
            }
        }
        results.append(qMakePair(interface, report));
    }
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

    printButton = new QPushButton(tr("Print"));
    printButton->setEnabled(false);
    connect(printButton, SIGNAL(clicked()), this, SLOT(print()));
    layout->addWidget(printButton);

    setLayout(layout);
    setMinimumSize(QSize(450, 400));

    /* show system info */
    text->setText("<h1>Diagnostics for " + osName() + "</h1>");

    /* get network info */
    networkThread = new NetworkThread(this);
    connect(networkThread, SIGNAL(finished()), this, SLOT(networkFinished()));
    networkThread->start();
}

void Diagnostics::print()
{
    QPrinter printer;
    QPrintDialog dialog(&printer);
    if (dialog.exec() == QDialog::Accepted)
        text->print(&printer);
}

void Diagnostics::networkFinished()
{
    foreach (const NetworkResult &pair, networkThread->results)
    {
        QNetworkInterface interface(pair.first);

        QString info = "<h2>Network interface " + interfaceName(interface) + QString("</h2>");
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
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    info += "<li>" + protocol + " gateway: " + pair.second.gateway_address.toString();
                    if (pair.second.gateway_ping)
                    {
                        const PingReport &report = pair.second.gateway_report;
                        info += QString(" (min: %1 ms, max: %2 ms, avg: %3 ms)").
                            arg(report.minRtt).arg(report.maxRtt).arg(report.averageRtt);
                    } else
                        info += " (unreachable)";
                    info += "</li>";
                }
            }
            info += "</ul>";
        } else {
            info += "<p>No address</p>";
        }
        text->append(info);
    }

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

    /* enable print button */
    printButton->setEnabled(true);
}

