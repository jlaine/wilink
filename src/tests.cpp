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

#include <QCoreApplication>
#include <QtTest/QtTest>

#include "updates.h"
#include "tests.h"

void TestUpdates::compareVersions()
{
    QVERIFY(Updates::compareVersions("1.0", "1.0") == 0);

    QVERIFY(Updates::compareVersions("2.0", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "2.0") == -1);

    QVERIFY(Updates::compareVersions("1.0.1", "1.0") == 1);
    QVERIFY(Updates::compareVersions("1.0", "1.0.1") == -1);

    QVERIFY(Updates::compareVersions("1.0.0", "0.99.7") == 1);
    QVERIFY(Updates::compareVersions("0.99.7", "1.0.0") == -1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TestUpdates testUpdates;
    QTest::qExec(&testUpdates);
}

