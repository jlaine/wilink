#include "wireless.h"

WirelessInterface::WirelessInterface(const QString &name)
    : interfaceName(name)
{
}

bool WirelessInterface::isValid() const
{
    return false;
}

QList<WirelessNetwork> WirelessInterface::networks()
{
    return QList<WirelessNetwork>();
}

