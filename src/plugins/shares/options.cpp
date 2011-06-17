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

#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QTreeView>

#include "plugins/application.h"
#include "plugins/shares.h"
#include "options.h"

ShareOptions::ShareOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;
    QGroupBox *sharesGroup = new QGroupBox(tr("Shared folders"));
    QVBoxLayout *vbox = new QVBoxLayout;

    QLabel *sharesLabel = new QLabel(tr("Select the folders you want to share. The files you share will only be visible in your residence, they can never be accessed outside your residence."));
    sharesLabel->setWordWrap(true);
    vbox->addWidget(sharesLabel);

    // full view
    m_fsModel = new ShareFolderModel(this);
    m_fsModel->setForcedFolder(wApp->sharesLocation());
    m_fsModel->setSelectedFolders(wApp->sharesDirectories());
    m_fsView = new QTreeView;
    m_fsView->setModel(m_fsModel);
    m_fsView->setColumnHidden(1, true);
    m_fsView->setColumnHidden(2, true);
    m_fsView->setColumnHidden(3, true);
    m_fsView->hide();
    vbox->addWidget(m_fsView);

    // simple view
    m_placesModel = new SharePlaceModel(this);
    m_placesModel->setSourceModel(m_fsModel);
    m_placesView = new QTreeView;
    m_placesView->setModel(m_placesModel);
    vbox->addWidget(m_placesView);

    QHBoxLayout *toggleBox = new QHBoxLayout;
    toggleBox->setMargin(0);
    toggleBox->addStretch();
    m_moreButton = new QPushButton(tr("More folders.."));
    connect(m_moreButton, SIGNAL(clicked()), this, SLOT(moreFolders()));
    toggleBox->addWidget(m_moreButton);
    m_fewerButton = new QPushButton(tr("Fewer folders.."));
    m_fewerButton->hide();
    connect(m_fewerButton, SIGNAL(clicked()), this, SLOT(fewerFolders()));
    toggleBox->addWidget(m_fewerButton);
    vbox->addLayout(toggleBox);

    sharesGroup->setLayout(vbox);
    layout->addWidget(sharesGroup);

    // downloads folder
    QGroupBox *downloadsGroup = new QGroupBox(tr("Downloads folder"));
    vbox = new QVBoxLayout;

    QLabel *downloadsLabel = new QLabel(tr("Select the folder in which received files will be stored."));
    downloadsLabel->setWordWrap(true);
    vbox->addWidget(downloadsLabel);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(0);
    m_directoryEdit = new QLineEdit;
    m_directoryEdit->setText(wApp->sharesLocation());
    m_directoryEdit->setEnabled(false);
    hbox->addWidget(m_directoryEdit);
    QPushButton *directoryButton = new QPushButton;
    directoryButton->setIcon(QIcon(":/album.png"));
    connect(directoryButton, SIGNAL(clicked()), this, SLOT(browse()));
    hbox->addWidget(directoryButton);
    vbox->addLayout(hbox);

    downloadsGroup->setLayout(vbox);
    layout->addWidget(downloadsGroup);

    setLayout(layout);
    setObjectName("shares");
    setWindowIcon(QIcon(":/share.png"));
    setWindowTitle(tr("Shares"));
}

void ShareOptions::browse()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setDirectory(m_directoryEdit->text());
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setWindowTitle(tr("Shares folder"));
    dialog->show();

    bool check;
    check = connect(dialog, SIGNAL(finished(int)),
                    dialog, SLOT(deleteLater()));
    Q_ASSERT(check);

    check = connect(dialog, SIGNAL(fileSelected(QString)),
                    this, SLOT(directorySelected(QString)));
    Q_ASSERT(check);
}

void ShareOptions::directorySelected(const QString &path)
{
    m_directoryEdit->setText(path);
    m_fsModel->setForcedFolder(path);
}

void ShareOptions::fewerFolders()
{
    m_fsView->hide();
    m_placesView->show();
    m_fewerButton->hide();
    m_moreButton->show();
}

void ShareOptions::moreFolders()
{
    m_placesView->hide();
    m_fsView->show();
    m_moreButton->hide();
    m_fewerButton->show();
}

bool ShareOptions::save()
{
    const QString path = m_directoryEdit->text();
    const QStringList mapped = m_fsModel->selectedFolders();

    // remember directory
    wApp->setSharesLocation(path);
    wApp->setSharesDirectories(mapped);
    return true;
}

void ShareOptions::scrollToHome()
{
    // scroll to home
    QModelIndex homeIndex = m_fsModel->index(QDir::homePath());
    m_fsView->setExpanded(homeIndex, true);
    m_fsView->scrollTo(homeIndex, QAbstractItemView::PositionAtTop);
}

void ShareOptions::showEvent(QShowEvent *event)
{
    ChatPreferencesTab::showEvent(event);
    QTimer::singleShot(0, this, SLOT(scrollToHome()));
}

