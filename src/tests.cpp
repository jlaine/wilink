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
#include <QtTest/QtTest>

#include "updates.h"
#include "utils.h"
#include "tests.h"

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

void TestUpdates::compareVersions()
{
    QVERIFY(Updates::compareVersions("1.0", "1.0") == 0);

    QVERIFY(Updates::compareVersions("2.0", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "2.0") == -1);

    QVERIFY(Updates::compareVersions("1.0.1", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "1.0.1") == -1);

    QVERIFY(Updates::compareVersions("1.0.0", "0.99.7") == 1);
    QVERIFY(Updates::compareVersions("0.99.7", "1.0.0") == -1);

    QVERIFY(Updates::compareVersions("0.9.5", "0.9.401") == 1);
    QVERIFY(Updates::compareVersions("0.9.401", "0.9.5") == -1);

    QVERIFY(Updates::compareVersions("0.9.402", "0.9.401") == 1);
    QVERIFY(Updates::compareVersions("0.9.401", "0.9.402") == -1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TestIndent testIndent;
    QTest::qExec(&testIndent);

    TestUpdates testUpdates;
    QTest::qExec(&testUpdates);
}

