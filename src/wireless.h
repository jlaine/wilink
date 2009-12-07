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

class WirelessInterface
{
public:
    WirelessInterface(const QString &interfaceName);
    bool isValid() const;
    QList<WirelessNetwork> networks();

private:
    QString interfaceName;
};

