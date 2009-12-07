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

#include <QList>
#include <QString>

class WirelessNetwork
{
public:
    int cinr() const { return w_cinr; };
    int rssi() const { return w_rssi; };
    QString ssid() const { return w_ssid; };

    void setCinr(int cinr) { w_cinr = cinr; };
    void setRssi(int rssi) { w_rssi = rssi; };
    void setSsid(const QString &ssid) { w_ssid = ssid; };

private:
    int w_cinr;
    int w_rssi;
    QString w_ssid;
};

class WirelessInterfacePrivate;

class WirelessInterface
{
public:
    WirelessInterface(const QString &interfaceName);
    ~WirelessInterface();

    bool isValid() const;
    QList<WirelessNetwork> networks();

private:
    QString interfaceName;
    WirelessInterfacePrivate *d;
};

