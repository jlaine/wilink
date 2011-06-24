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

UpdateDialog::UpdateDialog(QWidget *parent)
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
    layout->addLayout(hbox);

    /* progress */
    progressBar = new QProgressBar;
    progressBar->hide();
    layout->addWidget(progressBar);

    /* button box */
    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    layout->addWidget(buttonBox);

    setLayout(layout);
    setWindowTitle(tr("%1 software update").arg(qApp->applicationName()));

    /* updates */
    m_updates = new Updater(this);

    bool check;
    check = connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
                    this, SLOT(buttonClicked(QAbstractButton*)));
    Q_ASSERT(check);

    check = connect(m_updates, SIGNAL(error(Updater::Error, const QString&)),
                    this, SLOT(error(Updater::Error, const QString&)));
    Q_ASSERT(check);

    progressBar->setMaximum(m_updates->progressMaximum());
    progressBar->setValue(m_updates->progressValue());
    check = connect(m_updates, SIGNAL(progressValueChanged(int)),
                    progressBar, SLOT(setValue(int)));
    Q_ASSERT(check);

    check = connect(m_updates, SIGNAL(stateChanged(Updater::State)),
                    this, SLOT(_q_stateChanged(Updater::State)));
    Q_ASSERT(check);
}

void UpdateDialog::buttonClicked(QAbstractButton *button)
{
    if (buttonBox->standardButton(button) == QDialogButtonBox::Yes)
    {
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        m_updates->install();
    }
    else if (buttonBox->standardButton(button) == QDialogButtonBox::No)
    {
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        statusLabel->setText(tr("Not installing update."));
        hide();
    }
    else
    {
        hide();
    }
}

void UpdateDialog::check()
{
    show();
    m_updates->check();
}

void UpdateDialog::downloadProgress(qint64 done, qint64 total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(done);
}

void UpdateDialog::error(Updater::Error error, const QString &errorString)
{
    Q_UNUSED(error);

    statusLabel->setText(tr("Could not run software update, please try again later.") + "\n\n" + errorString);
}

Updater *UpdateDialog::updater() const
{
    return m_updates;
}

void UpdateDialog::_q_stateChanged(Updater::State state)
{
    if (state == Updater::CheckState) {
        statusLabel->setText(tr("Checking for updates.."));
        progressBar->hide();
    } else if (state == Updater::CheckState) {
        statusLabel->setText(tr("Downloading update.."));
        progressBar->show();
    } else if (state == Updater::InstallState) {
        statusLabel->setText(tr("Installing update.."));
        progressBar->hide();
    } else if (state == Updater::PromptState) {
        statusLabel->setText(QString("<p>%1</p><p><b>%2</b></p><pre>%3</pre><p>%4</p>")
                .arg(tr("Version %1 of %2 is available. Do you want to install it?")
                    .arg(m_updates->updateVersion())
                    .arg(qApp->applicationName()))
                .arg(tr("Changes:"))
                .arg(m_updates->updateChanges())
                .arg(tr("%1 will automatically exit to allow you to install the new version.")
                    .arg(qApp->applicationName())));
        progressBar->hide();
        buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
        show();
        buttonBox->button(QDialogButtonBox::Yes)->setFocus();
    } else {
        statusLabel->setText(tr("Your version of %1 is up to date.").arg(qApp->applicationName()));
        progressBar->hide();
    }
    if (isVisible())
        resize(size().expandedTo(sizeHint()));
}

