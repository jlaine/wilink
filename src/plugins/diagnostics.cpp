/*
 * wiLink
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

#include <QCoreApplication>
#include <QDateTime>
#include <QDomElement>
#include <QLayout>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QProgressBar>
#include <QShortcut>
#include <QTextBrowser>
#include <QTimer>
#include <QThread>

#include "qnetio/mime.h"
#include "qnetio/wallet.h"

#include "QXmppUtils.h"

#include "chat.h"
#include "chat_plugin.h"
#include "systeminfo.h"

#include "diagnostics.h"

#define DIAGNOSTICS_ROSTER_ID "0_diagnostics"

const char* ns_diagnostics = "http://wifirst.net/protocol/diagnostics";

static const QHostAddress serverAddress("213.91.4.201");

Q_DECLARE_METATYPE(QList<QHostInfo>)
Q_DECLARE_METATYPE(QList<Ping>)
Q_DECLARE_METATYPE(QList<Traceroute>)
Q_DECLARE_METATYPE(Interface)

static int id1 = qRegisterMetaType< QList<QHostInfo> >();
static int id2 = qRegisterMetaType< QList<Ping> >();
static int id3 = qRegisterMetaType< QList<Traceroute> >();
static int id4 = qRegisterMetaType< Interface >();

/* NETWORK */

void DiagnosticsThread::run()
{
    DiagnosticsIq iq;
    iq.setId(m_id);
    iq.setTo(m_to);
    iq.setType(QXmppIq::Result);

    QList<QHostAddress> gateways;
    QList<QHostInfo> lookups;
    int done = 0;
    int total = 3;

    emit progress(done, total);

    /* discover interfaces */
    QList<Interface> interfaceResults;
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (interface.flags() & QNetworkInterface::IsLoopBack)
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
        emit interfaceResult(result);
        emit progress(++done, ++total);
    }
    iq.setInterfaces(interfaceResults);

    /* try to detemine gateways */
    foreach (const QNetworkInterface &interface, QNetworkInterface::allInterfaces())
    {
        if (!(interface.flags() & QNetworkInterface::IsRunning) || (interface.flags() & QNetworkInterface::IsLoopBack))
            continue;

        foreach (const QNetworkAddressEntry &entry, interface.addressEntries())
        {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.netmask().isNull() && entry.netmask() != QHostAddress::Broadcast)
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
    emit progress(++done, total);

    /* run ping tests */
    QList<Ping> pings;
    foreach (const QHostAddress &gateway, gateways)
    {
        const Ping &ping = NetworkInfo::ping(gateway, 3);
        pings.append(ping);
        if (longPing.contains(gateway) && ping.sentPackets != ping.receivedPackets)
            pings.append(NetworkInfo::ping(gateway, 30));
    }
    emit pingResults(pings);
    emit progress(++done, total);
    iq.setPings(pings);

    /* run traceroute */
    QList<Traceroute> traceroutes;
    traceroutes << NetworkInfo::traceroute(serverAddress, 3, 4);
    emit tracerouteResults(traceroutes);
    emit progress(++done, total);
    iq.setTraceroutes(traceroutes);

    emit results(iq);
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
        row << (report.hostAddress.isNull() ? "-" : report.hostAddress.toString());
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
    : ChatPanel(parent), displayed(false), m_thread(NULL)
{
    /* prepare network access manager */
    network = new QNetworkAccessManager(this);
    connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
        QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));

    /* build user interface */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(headerLayout());
    text = new QTextBrowser;
    layout->addWidget(text);

    QHBoxLayout *hbox = new QHBoxLayout;

    progressBar = new QProgressBar;
    hbox->addWidget(progressBar);

    sendButton = new QPushButton(tr("Send"));
    sendButton->hide();
    connect(sendButton, SIGNAL(clicked()), this, SLOT(send()));
    hbox->addWidget(sendButton);

    refreshButton = new QPushButton(tr("Refresh"));
    refreshButton->setIcon(QIcon(":/refresh.png"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    hbox->addWidget(refreshButton);

    layout->addLayout(hbox);
    setLayout(layout);
    setMinimumSize(QSize(450, 400));
    setWindowIcon(QIcon(":/diagnostics.png"));
    setWindowTitle(tr("Diagnostics"));

    connect(this, SIGNAL(showPanel()), this, SLOT(slotShow()));
    connect(this, SIGNAL(hidePanel()), this, SIGNAL(unregisterPanel()));
}

void Diagnostics::addItem(const QString &title, const QString &value)
{
    text->append(QString("<h3>%1</h3>%2").arg(title, value));
}

void Diagnostics::addSection(const QString &title)
{
    text->append(QString("<h2>%1</h2>").arg(title));
}

void Diagnostics::refresh()
{
    if (m_thread)
        return;

    sendButton->setEnabled(false);
    refreshButton->setEnabled(false);

    /* show system info */
    text->setText("<h2>System information</h2>");
    TextList list;
    list << QString("Operating system: %1 %2").arg(SystemInfo::osName(), SystemInfo::osVersion());
    list << QString("%1: %2").arg(qApp->applicationName(), qApp->applicationVersion());
    list << QString("Date: %1").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss"));
    addItem("Environment", list.render());

    /* run network tests */
    m_thread = new DiagnosticsThread(this);
    connect(m_thread, SIGNAL(finished()), this, SLOT(networkFinished()));
    connect(m_thread, SIGNAL(dnsResults(const QList<QHostInfo> &)), this, SLOT(showDns(const QList<QHostInfo> &)));
    connect(m_thread, SIGNAL(interfaceResult(Interface)),this, SLOT(showInterface(Interface)));
    connect(m_thread, SIGNAL(pingResults(QList<Ping>)), this, SLOT(showPing(QList<Ping>)));
    connect(m_thread, SIGNAL(progress(int, int)), this, SLOT(showProgress(int, int)));
    connect(m_thread, SIGNAL(tracerouteResults(QList<Traceroute>)), this, SLOT(showTraceroute(QList<Traceroute>)));
    m_thread->start();
}

void Diagnostics::networkFinished()
{
    m_thread->wait();
    delete m_thread;
    m_thread = NULL;

    /* enable buttons */
    refreshButton->setEnabled(true);
    sendButton->setEnabled(true);
}

void Diagnostics::send()
{
    QNetIO::MimeForm form;
    form.addString("dump", text->toHtml());

    QNetworkRequest req(diagnosticsUrl.toString());
    req.setRawHeader("Content-Type", "multipart/form-data; boundary=" + form.boundary);
    network->post(req, form.render());
}

void Diagnostics::setUrl(const QUrl &url)
{
    diagnosticsUrl = url;
    if(url.isValid())
        sendButton->show();
}

void Diagnostics::slotShow()
{
    if (displayed)
        return;
    refresh();
    displayed = true;
}

void Diagnostics::showDns(const QList<QHostInfo> &results)
{
    addSection("Tests");

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

void Diagnostics::showProgress(int done, int total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

void Diagnostics::showTraceroute(const QList<Traceroute> &results)
{
    foreach (const Traceroute &traceroute, results)
        addItem("Traceroute", dumpPings(traceroute));
}

void Diagnostics::showInterface(const Interface &result)
{
    const bool showCinr = false;
    addSection("Network interface " + result.name());

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
    addItem("Addresses",
        list.size() ? list.render() : "<p>No address</p>");

    // wireless
    if (result.wirelessStandards())
    {
        QStringList supported;
        if (result.wirelessStandards() & Wireless_80211A)
            supported << "A";
        if (result.wirelessStandards() & Wireless_80211B)
            supported << "B";
        if (result.wirelessStandards() & Wireless_80211G)
            supported << "G";
        if (result.wirelessStandards() & Wireless_80211N)
            supported << "N";
        addItem("Wireless standards", supported.join(","));
    }

    if (!result.wirelessNetworks().isEmpty())
    {
        TextTable table;
        TextRow titles(true);
        titles << "SSID" << "RSSI";
        if (showCinr)
            titles << "CINR";
        table << titles;
        foreach (const WirelessNetwork &network, result.wirelessNetworks())
        {
            TextRow row;
            row << network.ssid();
            row << QString::number(network.rssi()) + " dBm";
            if (showCinr)
                row << (network.cinr() ? QString::number(network.cinr()) : QString());
            table << row;
        }
        addItem("Wireless networks", table.render());
    }
}

// EXTENSION

DiagnosticsExtension::DiagnosticsExtension(QXmppClient *client)
    : m_thread(0)
{
    qRegisterMetaType<DiagnosticsIq>("DiagnosticsIq");
}

DiagnosticsExtension::~DiagnosticsExtension()
{
    if (m_thread)
    {
        m_thread->wait();
        delete m_thread;
    }
}

void DiagnosticsExtension::handleResults(const DiagnosticsIq &results)
{
    client()->sendPacket(results);
}

bool DiagnosticsExtension::handleStanza(const QDomElement &stanza)
{
    if (stanza.tagName() == "iq" && DiagnosticsIq::isDiagnosticsIq(stanza))
    {
        DiagnosticsIq iq;
        iq.parse(stanza);

        if (iq.type() == QXmppIq::Get)
        {
            // only allow one request at a time
            if (m_thread && m_thread->isRunning())
            {
                DiagnosticsIq response;
                response.setType(QXmppIq::Error);
                response.setId(iq.id());
                response.setTo(iq.from());
                client()->sendPacket(response);
                return true;
            } else if (!m_thread) {
                m_thread = new DiagnosticsThread(this);
                connect(m_thread, SIGNAL(results(DiagnosticsIq)),
                        this, SLOT(handleResults(DiagnosticsIq)));
            }
            m_thread->setId(iq.id());
            m_thread->setTo(iq.from());
            m_thread->start();
        }
        return true;
    }
    return false;
}

// SERIALISATION

QList<Ping> DiagnosticsIq::pings() const
{
    return m_pings;
}

void DiagnosticsIq::setPings(const QList<Ping> &pings)
{
    m_pings = pings;
}

QList<Traceroute> DiagnosticsIq::traceroutes() const
{
    return m_traceroutes;
}

void DiagnosticsIq::setTraceroutes(const QList<Traceroute> &traceroutes)
{
    m_traceroutes = traceroutes;
}

QList<Interface> DiagnosticsIq::interfaces() const
{
    return m_interfaces;
}

void DiagnosticsIq::setInterfaces(QList<Interface> &interfaces)
{
    m_interfaces = interfaces;
}

bool DiagnosticsIq::isDiagnosticsIq(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement("query");
    return (queryElement.namespaceURI() == ns_diagnostics);
}

void DiagnosticsIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    while (!child.isNull())
    {
        if (child.tagName() == QLatin1String("ping"))
        {
            Ping ping;
            ping.parse(child);
            m_pings << ping;
        }
        else if (child.tagName() == QLatin1String("traceroute"))
        {
            Traceroute traceroute;
            traceroute.parse(child);
            m_pings << traceroute;
        }
        else if (child.tagName() == QLatin1String("wirelessInterface"))
        {
            Interface interface;
            interface.parse(child);
            m_interfaces << interface;
        }
        child = child.nextSiblingElement();
    }
}

void DiagnosticsIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("query");
    helperToXmlAddAttribute(writer, "xmlns", ns_diagnostics);
    foreach (const Ping &ping, m_pings)
        ping.toXml(writer);
    foreach (const Traceroute &traceroute, m_traceroutes)
        traceroute.toXml(writer);
    foreach (const Interface &interface, m_interfaces)
        interface.toXml(writer);
    writer->writeEndElement();
}

// PLUGIN

class DiagnosticsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool DiagnosticsPlugin::initialize(Chat *chat)
{
    /* add diagnostics extension */
    const QString domain = chat->client()->configuration().domain();
    if (domain == "wifirst.net")
    {
        DiagnosticsExtension *extension = new DiagnosticsExtension(chat->client());
        chat->client()->addExtension(extension);
    }

    /* register panel */
    Diagnostics *diagnostics = new Diagnostics;
    diagnostics->setObjectName(DIAGNOSTICS_ROSTER_ID);
    chat->addPanel(diagnostics);

    /* add menu entry */
    QList<QAction*> actions = chat->fileMenu()->actions();
    QAction *firstAction = actions.isEmpty() ? 0 : actions.first();
    QAction *action = new QAction(QIcon(":/diagnostics.png"), diagnostics->windowTitle(), chat->fileMenu());
    chat->fileMenu()->insertAction(firstAction, action);
    connect(action, SIGNAL(triggered()), diagnostics, SIGNAL(showPanel()));

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_I), chat);
    connect(shortcut, SIGNAL(activated()), diagnostics, SIGNAL(showPanel()));
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(diagnostics, DiagnosticsPlugin)
