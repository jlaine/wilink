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

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>

#include "systeminfo.h"
#include "updatesdialog.h"

UpdatesDialog::UpdatesDialog(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    /* status */
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setSpacing(20);
    QLabel *statusIcon = new QLabel;
    statusIcon->setPixmap(QPixmap(":/wiLink-64.png"));
    hbox->addWidget(statusIcon);
    statusLabel = new QLabel;
    statusLabel->setWordWrap(true);
    hbox->addWidget(statusLabel);
    layout->addItem(hbox);

    /* progress */
    progressBar = new QProgressBar;
    progressBar->hide();
    layout->addWidget(progressBar);

    /* button box */
    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Yes);
    buttonBox->addButton(QDialogButtonBox::No);
    buttonBox->hide();
    layout->addWidget(buttonBox);

    setLayout(layout);
    setWindowTitle(tr("%1 software update").arg(qApp->applicationName()));

    /* updates */
    updates = new Updates(this);
    updates->setCacheDirectory(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));

    bool check;
    check = connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
                    this, SLOT(buttonClicked(QAbstractButton*)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(checkStarted()),
                    this, SLOT(checkStarted()));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(checkFinished(Release)),
                    this, SLOT(checkFinished(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(downloadFinished(Release)),
                    this, SLOT(downloadFinished(Release)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(error(Updates::UpdatesError, const QString&)),
                    this, SLOT(error(Updates::UpdatesError, const QString&)));
    Q_ASSERT(check);

    check = connect(updates, SIGNAL(downloadProgress(qint64, qint64)),
                    this, SLOT(downloadProgress(qint64, qint64)));
    Q_ASSERT(check);
}

void UpdatesDialog::buttonClicked(QAbstractButton *button)
{
    buttonBox->hide();
    if (buttonBox->standardButton(button) == QDialogButtonBox::Yes)
    {
        statusLabel->setText(tr("Installing update.."));
        updates->install(promptRelease);
    } else {
        statusLabel->setText(tr("Not installing update."));
        hide();
    }
}

void UpdatesDialog::check()
{
    show();
    updates->check();
}

void UpdatesDialog::checkStarted()
{
    statusLabel->setText(tr("Checking for updates.."));
}

void UpdatesDialog::checkFinished(const Release &release)
{
    if (release.isValid())
    {
        statusLabel->setText(tr("Downloading update.."));
        progressBar->show();
        updates->download(release);
    } else {
        statusLabel->setText(tr("Your version of %1 is up to date.").arg(qApp->applicationName()));
    }
}

void UpdatesDialog::downloadProgress(qint64 done, qint64 total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

void UpdatesDialog::downloadFinished(const Release &release)
{
    progressBar->hide();
    promptRelease = release;
    statusLabel->setText(QString("<p>%1</p><p><b>%2</b></p><pre>%3</pre><p>%4</p>")
            .arg(tr("Version %1 of %2 is available. Do you want to install it?")
                .arg(release.version)
                .arg(release.package))
            .arg(tr("Changes:"))
            .arg(release.changes)
            .arg(tr("%1 will automatically exit to allow you to install the new version.")
                .arg(release.package)));
    buttonBox->show();
    show();
    buttonBox->button(QDialogButtonBox::Yes)->setFocus();
}

void UpdatesDialog::error(Updates::UpdatesError error, const QString &errorString)
{
    statusLabel->setText(tr("Could not run software update, please try again later.") + "\n\n" + errorString);
}

