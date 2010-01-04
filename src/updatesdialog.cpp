/*
 * wDesktop
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

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QProgressBar>

#include "updatesdialog.h"

UpdatesDialog::UpdatesDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    /* status */
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *statusIcon = new QLabel;
    statusIcon->setPixmap(QPixmap(":/wDesktop.png"));
    hbox->addWidget(statusIcon);
    statusLabel = new QLabel;
    hbox->addWidget(statusLabel);
    layout->addItem(hbox);

    /* progress */
    progressBar = new QProgressBar;
    layout->addWidget(progressBar);
    setLayout(layout);

    /* check for updates */
    updates = new Updates(this);
    connect(updates, SIGNAL(updateAvailable(const Release&)), this, SLOT(updateAvailable(const Release&)));
    connect(updates, SIGNAL(updateDownloaded(const QUrl&)), this, SLOT(updateDownloaded(const QUrl&)));
    connect(updates, SIGNAL(updateFailed()), this, SLOT(updateFailed()));
    connect(updates, SIGNAL(updateProgress(qint64, qint64)), this, SLOT(updateProgress(qint64, qint64)));
}

void UpdatesDialog::check()
{
    updates->check();
}

void UpdatesDialog::setUrl(const QUrl &url)
{
    updates->setUrl(url);
}

void UpdatesDialog::updateAvailable(const Release &release)
{
    const QString message = tr("Version %1 of %2 is available. Do you want to install it?")
            .arg(release.version)
            .arg(release.package);
    if (QMessageBox::question(NULL,
        tr("Update available"),
        QString("<p>%1</p><p><b>%2</b></p><pre>%3</pre>").arg(message, tr("Changes:"), release.changes),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        QString downloadDir = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
        statusLabel->setText(tr("Downloading"));
        show();
        updates->download(release, downloadDir);
    }
}

void UpdatesDialog::updateDownloaded(const QUrl &url)
{
    QDesktopServices::openUrl(url);
    qApp->quit();
}

void UpdatesDialog::updateFailed()
{

}

void UpdatesDialog::updateProgress(qint64 done, qint64 total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

