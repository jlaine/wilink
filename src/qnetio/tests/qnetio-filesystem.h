/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <QObject>
#include <QFile>
#include <QStringList>

#include "filesystem.h"

using namespace QNetIO;

class BocTestFs : public QObject
{
    Q_OBJECT

public:
    BocTestFs(const QString &testUrl, QObject *parent);
    void run();

private slots:
    void jobFinished(FileSystemJob *job);

signals:
    void result(bool outcome, const QString &cmd, const QString &reason);
    void done();

private:
    void start(const QString &op, const QUrl &arg);

private:
    FileSystem *fs;
    int step;
    QUrl url;
    QString subdirName;
    QUrl subdirUrl;
    QFile putFile;
    QUrl putUrl;
    QUrl getUrl;

    QString currentOp;
    QString currentTest;
};

class BocTestFsHarness : public QObject
{
    Q_OBJECT

public:
    BocTestFsHarness(const QStringList &testUrls);

public slots:
    void result(bool outcome, const QString &cmd, const QString &reason);
    void run();

private:
    int index;
    int failed;
    int passed;
    QStringList urls;
    BocTestFs *test;
};
