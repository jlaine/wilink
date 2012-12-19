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

#include <getopt.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QTextStream>

#include "qnetio-filesystem.h"
#include "wallet.h"

QTextStream cout(stdout);

enum steps
{
    STEP_OPEN,
    STEP_LIST_TOP,
    STEP_MKDIR,
    STEP_LIST_TOP_AFTER_MKDIR,
    STEP_LIST_SUB,
    STEP_PUT,
    STEP_LIST_SUB_AFTER_PUT,
    STEP_GET,
    STEP_REMOVE,
    STEP_LIST_SUB_AFTER_REMOVE,
    STEP_RMDIR,
    STEP_LIST_TOP_AFTER_RMDIR
};

BocTestFs::BocTestFs(const QString &testUrl, QObject *parent)
    : QObject(parent),
    fs(NULL),
    step(-1),
    url(testUrl),
    subdirName("boctest"),
    putFile("boctest.jpg")
{
    /* sub directory */
    subdirUrl = FileInfo::fileUrl(url, subdirName);

    /* put file */
    putUrl = FileInfo::fileUrl(subdirUrl, putFile.fileName());

    fs = FileSystem::create(url, this);
    connect(fs, SIGNAL(jobFinished(FileSystemJob*)),
            this, SLOT(jobFinished(FileSystemJob*)));
}

void BocTestFs::run()
{
    step++;
    switch (step)
    {
    case STEP_OPEN:
        start("open", url);
        fs->open(url);
        break;
    case STEP_LIST_TOP:
        start("list", url);
        fs->list(url);
        break;
    case STEP_MKDIR:
        start("mkdir", subdirUrl);
        fs->mkdir(subdirUrl);
        break;
    case STEP_LIST_TOP_AFTER_MKDIR:
        start("list", url);
        fs->list(url);
        break;
    case STEP_LIST_SUB:
        start("list", subdirUrl);
        fs->list(subdirUrl);
        break;
    case STEP_PUT:
        start("put", putUrl);
        putFile.open(QIODevice::ReadOnly);
        fs->put(putUrl, &putFile);
        break;
    case STEP_LIST_SUB_AFTER_PUT:
        start("list", subdirUrl);
        fs->list(subdirUrl);
        break;
    case STEP_GET:
        start("get", getUrl);
        fs->get(getUrl, QNetIO::FileSystem::FullSize);
        break;
    case STEP_REMOVE:
        start("remove", getUrl);
        fs->remove(getUrl);
        break;
    case STEP_LIST_SUB_AFTER_REMOVE:
        start("list", subdirUrl);
        fs->list(subdirUrl);
        break;
    case STEP_RMDIR:
        start("rmdir", subdirUrl);
        fs->rmdir(subdirUrl);
        break;
    case STEP_LIST_TOP_AFTER_RMDIR:
        start("list", url);
        fs->list(url);
        break;
    default:
        emit done();
    }
}

void BocTestFs::start(const QString &op, const QUrl &arg)
{
    currentOp = op;
    currentTest = currentOp + " " + arg.toString();
}

void BocTestFs::jobFinished(FileSystemJob *job)
{
    job->deleteLater();

    if (job->error())
    {
        emit result(false, currentTest, QString("command failed : %1").arg(job->errorString()));
        return;
    }

    // check command output
    if (job->operation() == FileSystemJob::Get)
    {
        const QByteArray data = job->readAll();
        if (data.isEmpty()) {
            emit result(false, currentTest, "get did not return data");
            return;
        }
    }
    else if (job->operation() == FileSystemJob::List)
    {
        bool foundSubdir = false;
        FileInfoList results = job->results();
        for (int i = 0; i < results.size(); i++)
        {
            //qDebug() << " " << results[i].name() << results[i].url();
            if (results[i].name() == subdirName)
                foundSubdir = true;
            if (step == STEP_LIST_SUB_AFTER_PUT)
                getUrl = results[i].url();
        }
        if ((step == STEP_LIST_TOP || step == STEP_LIST_TOP_AFTER_RMDIR) && foundSubdir)
        {
            emit result(false, currentTest, "found subdir");
            return;
        } else if (step == STEP_LIST_TOP_AFTER_MKDIR && !foundSubdir) {
            emit result(false, currentTest, "did not find subdir");
            return;
        } else if ((step == STEP_LIST_SUB || step == STEP_LIST_SUB_AFTER_REMOVE) && results.size()) {
            emit result(false, currentTest, "non empty subdir");
            return;
        } else if (step == STEP_LIST_SUB_AFTER_PUT && results.size() != 1) {
            emit result(false, currentTest, "wrong subdir contents");
            return;
        }
    }

    emit result(true, currentTest, QString());
    run();
}

BocTestFsHarness::BocTestFsHarness(const QStringList &testUrls)
    : index(-1), urls(testUrls)
{
}

void BocTestFsHarness::result(bool outcome, const QString &cmd, const QString &reason)
{
    if (outcome)
    {
        qDebug() << "PASS   :" << cmd;
        passed++;
    } else {
        qWarning() << "FAIL   :" << cmd << reason;
        failed++;
        run();
    }
}

void BocTestFsHarness::run()
{
    if (index >= 0)
    {
        qDebug() << "Totals:" << passed << "passed," << failed << "failed";
        qDebug() << "*********" << "Finished testing of" << urls[index] << "*********";
        qDebug();
    }

    index++;
    passed = 0;
    failed = 0;
    if (index >= urls.size())
    {
        qApp->quit();
    } else {
        qDebug() << "*********" << "Start testing of" << urls[index] << "*********";
        test = new BocTestFs(urls[index], this);
        connect(test, SIGNAL(result(bool,QString,QString)), this, SLOT(result(bool,QString,QString)));
        connect(test, SIGNAL(done()), this, SLOT(run()));
        test->run();
    }
}

void usage()
{
    QStringList backends;
    foreach (const Wallet::Backend &backend, Wallet::backends())
        backends << backend.key;

    cout << "Usage: qnetio-filesystem [options]" << endl << endl;
    cout << "Options" << endl;
    cout << " -b<backend>  use the <backend> wallet backend" << endl;
    cout << "              available: " << backends.join(", ") << endl;
    cout << " -h           display help message" << endl;
    cout << " -p<path>     add <path> to the plugin search path" << endl;
    cout << " -t<url>      run tests on the given url" << endl;
}

int main(int argc, char *argv[])
{
    char c;
    QStringList pluginPaths;
    QString backend;
    QString testUrl;
    enum {
        OP_NONE,
        OP_HELP,
        OP_TEST,
    } op = OP_NONE;

    /* parse command line arguments */
    while ((c = getopt(argc, argv, "b:hp:t:")) != -1)
    {
        if (c == 'h')
        {
            op = OP_HELP;
        } else if (c == 'b') {
            backend = QString::fromLocal8Bit(optarg);
        } else if (c == 'p') {
            pluginPaths << QString::fromLocal8Bit(optarg);
        } else if (c == 't') {
            op = OP_TEST;
            testUrl = QString::fromLocal8Bit(optarg);
        }
    }

    /* create application */
    QCoreApplication app(argc, argv);
    app.setApplicationName("QNetIO");
    if (!pluginPaths.isEmpty())
        app.setLibraryPaths(app.libraryPaths() + pluginPaths);

    if (op == OP_HELP)
    {
        usage();
        return EXIT_SUCCESS;
    }

    /* create wallet */
    Wallet *wallet = Wallet::create(backend);
    if (!wallet)
    {
        qDebug("No wallet was found to run the tests");
        return EXIT_FAILURE;
    }

    if (!QFileInfo("boctest.jpg").exists())
    {
        qDebug("You need an image called boctest.jpg to run these tests");
        return EXIT_FAILURE;
    }

    if (op == OP_TEST)
    {
        QStringList urls(testUrl);
/*
        urls << "gphoto:///";
        urls << "file:///tmp";
        urls << "picasa://default";
        urls << "https://mobile.bolloretelecom.eu/webdav";
        urls << "wifirst://default";
        urls << "ftp://localhost/incoming";
*/
        BocTestFsHarness test(urls);
        test.run();
        return app.exec();
    }
}

