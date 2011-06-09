/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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
#include <QDomDocument>
#include <QtTest/QtTest>

#include "QSoundFile.h"

#include "diagnostics/iq.h"
#include "plugins/updates.h"
#include "plugins/utils.h"
#include "tests.h"

template <class T>
static void parsePacket(T &packet, const QByteArray &xml)
{
    //qDebug() << "parsing" << xml;
    QDomDocument doc;
    QCOMPARE(doc.setContent(xml, true), true);
    QDomElement element = doc.documentElement();
    packet.parse(element);
}

template <class T>
static void serializePacket(T &packet, const QByteArray &xml)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    QXmlStreamWriter writer(&buffer);
    packet.toXml(&writer);
    qDebug() << "expect " << xml;
    qDebug() << "writing" << buffer.data();
    QCOMPARE(buffer.data(), xml);
}

void TestDiagnostics::testPacket()
{
    const QByteArray xml(
    "<iq type=\"result\">"
        "<query xmlns=\"http://wifirst.net/protocol/diagnostics\">"
            "<software type=\"os\" name=\"Windows\" version=\"XP\"/>"
            "<interface name=\"en1\">"
                "<address broadcast=\"\" ip=\"FE80:0:0:0:226:8FF:FEE1:A96B\" netmask=\"FFFF:FFFF:FFFF:FFFF:0:0:0:0\"/>"
                "<address broadcast=\"192.168.99.255\" ip=\"192.168.99.179\" netmask=\"255.255.255.0\"/>"
                "<wireless standards=\"ABGN\">"
                    "<network current=\"1\" cinr=\"-91\" rssi=\"-59\" ssid=\"Maki\"/>"
                    "<network cinr=\"-92\" rssi=\"-45\" ssid=\"freephonie\"/>"
                "</wireless>"
            "</interface>"
            "<lookup hostName=\"www.google.fr\">"
                "<address>2A00:1450:4007:800:0:0:0:68</address>"
                "<address>66.249.92.104</address>"
            "</lookup>"
            "<ping hostAddress=\"192.168.99.1\" minimumTime=\"1.657\" maximumTime=\"44.275\" averageTime=\"20.258\" sentPackets=\"3\" receivedPackets=\"3\"/>"
            "<traceroute hostAddress=\"213.91.4.201\">"
                "<ping hostAddress=\"192.168.99.1\" minimumTime=\"1.719\" maximumTime=\"4.778\" averageTime=\"3.596\" sentPackets=\"3\" receivedPackets=\"3\"/>"
            "</traceroute>"
        "</query>"
    "</iq>");

    DiagnosticsIq iq;
    parsePacket(iq, xml);
    serializePacket(iq, xml);
}

void TestIndent::indentCollapsed()
{
    QCOMPARE(indentXml("<sometag/>"),
        QString::fromLatin1("<sometag/>"));
    QCOMPARE(indentXml("<sometag/><othertag/>"),
        QString::fromLatin1("<sometag/>\n<othertag/>"));
    QCOMPARE(indentXml("<sometag/>\n<othertag/>"),
        QString::fromLatin1("<sometag/>\n<othertag/>"));
    QCOMPARE(indentXml("<sometag/>\n\n<othertag/>"),
        QString::fromLatin1("<sometag/>\n<othertag/>"));

    QCOMPARE(indentXml("<?xml version='1.0'?><sometag>"),
        QString::fromLatin1("<?xml version='1.0'?>\n<sometag>"));
}

void TestIndent::indentElement()
{
    QCOMPARE(indentXml("<sometag>value</sometag>"),
        QString::fromLatin1("<sometag>value</sometag>"));

    QCOMPARE(indentXml("<sometag><nested/></sometag>"),
        QString::fromLatin1("<sometag>\n    <nested/>\n</sometag>"));
    QCOMPARE(indentXml("<sometag><nested/><nested2/></sometag>"),
        QString::fromLatin1("<sometag>\n    <nested/>\n    <nested2/>\n</sometag>"));
    QCOMPARE(indentXml("<sometag><nested>value</nested></sometag>"),
        QString::fromLatin1("<sometag>\n    <nested>value</nested>\n</sometag>"));
}

void TestIndent::checkJid()
{
    QCOMPARE(isBareJid("foo"), false);
    QCOMPARE(isBareJid("foo@bar"), true);
    QCOMPARE(isBareJid("foo@bar/wiz"), false);
    QCOMPARE(isBareJid("foo@bar/wiz/woo"), false);
    QCOMPARE(isBareJid("foo/wiz"), false);

    QCOMPARE(isFullJid("foo"), false);
    QCOMPARE(isFullJid("foo@bar"), false);
    QCOMPARE(isFullJid("foo@bar/wiz"), true);
    QCOMPARE(isFullJid("foo@bar/wiz/woo"), false);
    QCOMPARE(isFullJid("foo/wiz"), false);
}

void TestSound::copyWav()
{
    const QString inputPath(":/tones.wav");
    const QString outputPath("output.wav");

    // read input
    QSoundFile input(inputPath);
    QCOMPARE(input.open(QIODevice::ReadOnly), true);
    const QByteArray data = input.readAll();
    input.close();

    // write output
    QSoundFile output(outputPath);
    output.setFormat(input.format());
    output.setMetaData(input.metaData());
    QCOMPARE(output.open(QIODevice::WriteOnly), true);
    output.write(data);
    output.close();

    // compare
    QFile inputFile(inputPath);
    QCOMPARE(inputFile.open(QIODevice::ReadOnly), true);
    QFile outputFile(outputPath);
    QCOMPARE(outputFile.open(QIODevice::ReadOnly), true);
    QCOMPARE(inputFile.readAll(), outputFile.readAll());
    outputFile.remove();
}

void TestSound::readWav()
{
    // read input
    QSoundFile input(":/tones.wav");
    QCOMPARE(input.open(QIODevice::ReadOnly), true);

    // check format
    QCOMPARE(input.format().channels(), 1);
    QCOMPARE(input.format().frequency(), 44100);
    QCOMPARE(input.format().sampleSize(), 16);

    // check info
    const QList<QPair<QSoundFile::MetaData, QString> > info = input.metaData();
    QCOMPARE(info.size(), 4);
    QCOMPARE(info[0].first, QSoundFile::TitleMetaData);
    QCOMPARE(info[0].second, QLatin1String("Track"));
    QCOMPARE(info[1].first, QSoundFile::ArtistMetaData);
    QCOMPARE(info[1].second, QLatin1String("Artist"));
    QCOMPARE(info[2].first, QSoundFile::DescriptionMetaData);
    QCOMPARE(info[2].second, QLatin1String("Comments"));
    QCOMPARE(info[3].first, QSoundFile::DateMetaData);
    QCOMPARE(info[3].second, QLatin1String("Year"));

    const QByteArray data = input.readAll();
    input.close();
}

void TestUpdates::compareVersions()
{
    QVERIFY(Updates::compareVersions("1.0", "1.0") == 0);

    QVERIFY(Updates::compareVersions("2.0", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "2.0") == -1);

    QVERIFY(Updates::compareVersions("1.0.1", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "1.0.1") == -1);

    QVERIFY(Updates::compareVersions("1.0.1", "1.0.0") == 1);
    QVERIFY(Updates::compareVersions("1.0.0", "1.1.1") == -1);

    QVERIFY(Updates::compareVersions("1.0.0", "0.99.7") == 1);
    QVERIFY(Updates::compareVersions("0.99.7", "1.0.0") == -1);

    QVERIFY(Updates::compareVersions("0.9.5", "0.9.4") == 1);
    QVERIFY(Updates::compareVersions("0.9.4", "0.9.5") == -1);

    QVERIFY(Updates::compareVersions("0.9.5", "0.9.401") == 1);
    QVERIFY(Updates::compareVersions("0.9.401", "0.9.5") == -1);

    QVERIFY(Updates::compareVersions("0.9.402", "0.9.401") == 1);
    QVERIFY(Updates::compareVersions("0.9.401", "0.9.402") == -1);

    QVERIFY(Updates::compareVersions("0.9.9a", "0.9.9") == 1);
    QVERIFY(Updates::compareVersions("0.9.9", "0.9.9a") == -1);

    QVERIFY(Updates::compareVersions("1.0.0", "0.9.9a913") == 1);
    QVERIFY(Updates::compareVersions("0.9.9a913", "1.0.0") == -1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int errors = 0;

    TestDiagnostics testDiagnostics;
    errors += QTest::qExec(&testDiagnostics);

    TestIndent testIndent;
    errors += QTest::qExec(&testIndent);

    TestSound testSound;
    errors += QTest::qExec(&testSound);

    TestUpdates testUpdates;
    errors += QTest::qExec(&testUpdates);

    if (errors) {
        qWarning("Total failed tests: %i", errors);
        return EXIT_FAILURE;
    }
}

