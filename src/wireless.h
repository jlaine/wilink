#include <QString>

class WirelessNetwork
{
public:
    QString ssid() const { return w_ssid; };
    void setSsid(const QString &ssid) { w_ssid = ssid; };

private:
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

