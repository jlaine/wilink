/*
 * wDesktop
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

#include <QLayout>
#include <QNetworkInterface>
#include <QPushButton>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextBrowser>
#include <QThread>

#include "config.h"
#include "diagnostics.h"
#include "systeminfo.h"

static const QString wdesktopVersion = QString::fromLatin1(WDESKTOP_VERSION);
static const QHostAddress serverAddress("213.91.4.201");

Q_DECLARE_METATYPE(QList<QHostInfo>)
Q_DECLARE_METATYPE(QList<Ping>)

int id = qRegisterMetaType< QList<QHostInfo> >();
int id2 = qRegisterMetaType< QList<Ping> >();

static QString interfaceName(const QNetworkInterface &interface)
{
#if QT_VERSION >= 0x040500
    return interface.humanReadableName();
#else
    return interface.name();
#endif
}

/* NETWORK */

void NetworkThread::run()
{
    QList<QHostAddress> gateways;
    QList<QHostInfo> lookups;

    /* try to detemine gateways */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.netmask().isNull())
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
    emit dnsResults(lookups);

    /* run ping tests */
    QList<Ping> pings;
    foreach (const QHostAddress &gateway, gateways)
        pings.append(NetworkInfo::ping(gateway, longPing.contains(gateway) ? 30 : 3));
    emit pingResults(pings);

    /* run traceroute */
    QList<Ping> traceroute = NetworkInfo::traceroute(serverAddress, 3, 4);
    emit tracerouteResults(traceroute);
}

/* WIRELESS */

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
        row.setColor(report.receivedPackets == report.sentPackets ? "green" : "red");
        row << report.hostAddress.toString();
        row << QString("%1 / %2").arg(report.receivedPackets).arg(report.sentPackets);
        if (report.receivedPackets)
        {
            row << QString("%1 ms").arg(report.minimumTime);
            row << QString("%1 ms").arg(report.maximumTime);
            row << QString("%1 ms").arg(report.averageTime);
        } else {
            row << "-" << "-" << "-";
        }
        table << row;
    }
    return table.render();
}

Diagnostics::Diagnostics(QWidget *parent)
    : QDialog(parent), networkThread(NULL), wirelessThread(NULL)
{
    QVBoxLayout *layout = new QVBoxLayout;

    text = new QTextBrowser;
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
    setWindowIcon(QIcon(":/diagnostics.png"));

    refresh();
}

void Diagnostics::addItem(const QString &title, const QString &value)
{
    text->append(QString("<h3>%1</h3>%2").arg(title, value));
}

void Diagnostics::addSection(const QString &title)
{
    text->append(QString("<h2>%1</h2>").arg(title));
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
    text->setText("<h2>System information</h2>");
    TextList list;
    list << QString("Operating system: %1 %2").arg(SystemInfo::osName(), SystemInfo::osVersion());
    list << QString("wDesktop: %1").arg(wdesktopVersion);
    addItem("Software version", list.render());

    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        TextList list;
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
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
        addItem("Interface " + interfaceName(interface),
            list.size() ? list.render() : "<p>No address</p>");
    }

    addSection("Tests");

    /* run network tests */
    networkThread = new NetworkThread(this);
    connect(networkThread, SIGNAL(finished()), this, SLOT(networkFinished()));
    connect(networkThread, SIGNAL(dnsResults(const QList<QHostInfo> &)), this, SLOT(showDns(const QList<QHostInfo> &)));
    connect(networkThread, SIGNAL(pingResults(const QList<Ping> &)), this, SLOT(showPing(const QList<Ping> &)));
    networkThread->start();
}

void Diagnostics::networkFinished()
{
    /* get wireless info */
    wirelessThread = new WirelessThread(this);
    connect(wirelessThread, SIGNAL(finished()), this, SLOT(wirelessFinished()));
    wirelessThread->start();
}

void Diagnostics::showDns(const QList<QHostInfo> &results)
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

void Diagnostics::showPing(const QList<Ping> &results)
{
    addItem("Ping", dumpPings(results));
}

void Diagnostics::showTraceroute(const QList<Ping> &results)
{
    addItem("Traceroute", dumpPings(results));
}

void Diagnostics::wirelessFinished()
{
    foreach (const WirelessResult &result, wirelessThread->results)
    {
        addSection("Wireless interface " + interfaceName(result.interface));

        if (result.currentNetwork.isValid())
        {
            TextList list;
            list << "SSID: " + result.currentNetwork.ssid();
            list << "RSSI: " + QString::number(result.currentNetwork.rssi());
            if (result.currentNetwork.cinr())
                list << "CINR: " + QString::number(result.currentNetwork.cinr());
            addItem("Current network", list.render());
        }

        TextTable table;
        TextRow titles(true);
        titles << "SSID" << "RSSI" << "CINR";
        table << titles;
        foreach (const WirelessNetwork &network, result.availableNetworks)
        {
            if (network != result.currentNetwork)
            {
                TextRow row;
                row << network.ssid();
                row << QString::number(network.rssi());
                row << (network.cinr() ? QString::number(network.cinr()) : QString());
                table << row;
            }
        }
        addItem("Available networks", table.render());
    }

    /* enable buttons */
    refreshButton->setEnabled(true);
    printButton->setEnabled(true);
}

