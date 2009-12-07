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

#include "diagnostics.h"
#include "wireless.h"

Diagnostics::Diagnostics(QWidget *parent)
    : QDialog(parent)
{
    text = new QTextEdit;

/*
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidget(text);
*/

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(text);
    setLayout(layout);

    /* system info */
#ifdef Q_OS_LINUX
    text->setText("OS: Linux");
#endif
#ifdef Q_OS_MAC
    text->setText("OS: Mac OS");
#endif
#ifdef Q_OS_WIN
    text->setText("OS: Windows");
#endif

    /* network info */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;
        QString info = QString("<b>Interface: ") + interface.humanReadableName() + QString("</b>");
        info += "<ul>";
        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            info += "<li>IP address: " + entry.ip().toString() + "</li>";
            info += "<li>Netmask: " + entry.netmask().toString() + "</li>";
        }
        WirelessInterface wireless(interface.humanReadableName());
        if (wireless.isValid())
        {
            foreach (const WirelessNetwork &network, wireless.networks())
                info += "<li>SSID " + network.ssid() + " (RSSI: " + QString::number(network.rssi()) + ", CINR: " + QString::number(network.cinr()) + ")";
        }
        info += "</ul>";
        text->append(info);
    }
}

