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

#include <QTextEdit>
#include <QLayout>
#include <QNetworkInterface>
#include <QScrollArea>
#include <QThread>

#include "diagnostics.h"
#include "wireless.h"

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

class WirelessThread : public QThread
{
public:
    WirelessThread(QObject *parent) : QThread(parent) {};
    void run();
    QHash<QString, QList<WirelessNetwork> > results;
};

void WirelessThread::run()
{
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        const QString &interfaceName = interface.name();
        WirelessInterface wireless(interfaceName);
        if (!wireless.isValid())
            continue;
        results[interfaceName] = wireless.networks();
    }
}

Diagnostics::Diagnostics(QWidget *parent)
    : QDialog(parent), wirelessThread(NULL)
{
    text = new QTextEdit;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(text);
    setLayout(layout);
    setMinimumSize(QSize(400, 300));

    /* system info */
    text->setText("<h1>Diagnostics for " + osName() + "</h1>");

    /* network info */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        QString info = "<h2>Network interface " + interface.humanReadableName() + QString("</h2>");
        if (interface.addressEntries().size())
        {
            info += "<ul>";
            foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
            {
                info += "<li>IP address: " + entry.ip().toString() + "</li>";
                info += "<li>Netmask: " + entry.netmask().toString() + "</li>";
            }
            info += "</ul>";
        } else {
            info += "<p>No address</p>";
        }
        text->append(info);
    }

    /* wireless info */
    wirelessThread = new WirelessThread(this);
    connect(wirelessThread, SIGNAL(finished()), this, SLOT(wirelessFinished()));
    wirelessThread->start();
}

void Diagnostics::wirelessFinished()
{
    QString info;
    foreach (const QString &interfaceName, wirelessThread->results.keys())
    {
        QNetworkInterface interface = QNetworkInterface::interfaceFromName(interfaceName);
        info += "<h2>Wireless interface " + interface.humanReadableName() + "</h2>";
        info += "<ul>";
        foreach (const WirelessNetwork &network, wirelessThread->results[interfaceName])
            info += "<li>SSID " + network.ssid() + " (RSSI: " + QString::number(network.rssi()) + ", CINR: " + QString::number(network.cinr()) + ")";
        info += "</ul>";
    }
    text->append(info);
}

