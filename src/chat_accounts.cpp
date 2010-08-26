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
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>

#include "qxmpp/QXmppUtils.h"

#include "chat_accounts.h"

ChatAccounts::ChatAccounts(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *helpLabel = new QLabel(
        tr("In addition to your %1 account, %2 can connect to additional chat accounts such as Google Talk and Facebook.")
            .arg(qApp->organizationName())
            .arg(qApp->applicationName()));
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    listWidget = new QListWidget;
    listWidget->setIconSize(QSize(32, 32));
    connect(listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(updateButtons()));
    layout->addWidget(listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addAccount()));
    buttonBox->addButton(addButton, QDialogButtonBox::ActionRole);

    removeButton = new QPushButton;
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeAccount()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);
    updateButtons();
}

QStringList ChatAccounts::accounts() const
{
    QStringList accounts;
    for (int i = 0; i < listWidget->count(); i++)
        accounts << listWidget->item(i)->text();
    return accounts;
}

void ChatAccounts::addAccount()
{
    bool valid = false;
    QString jid, error;
    while (!valid)
    {
        bool ok = false;
        QString prompt = tr("Enter the address of the account you want to add.");
        jid = QInputDialog::getText(this, tr("Add an account"),
                      tr("Enter the address of the account you want to add.") + (error.isEmpty() ? QString() : "\n\n" + error),
                      QLineEdit::Normal, jid, &ok).toLower();
        if (!ok)
            return;

        if (jid.count("@") != 1)
        {
            error = tr("The address you entered is invalid.");
            continue;
        }

        valid = true;
        const QString domain = jidToDomain(jid);
        for (int i = 0; i < listWidget->count(); i++)
        {
            if (jidToDomain(listWidget->item(i)->text()) == domain)
            {
                error = tr("You already have an account for '%1'.").arg(domain);
                valid = false;
                break;
            }
        }
    }

    addEntry(jid);
}

void ChatAccounts::addEntry(const QString &jid)
{
    QListWidgetItem *wdgItem = new QListWidgetItem(QIcon(":/chat.png"), jid);
    // FIXME : do accounts need to be locked ?
    wdgItem->setData(Qt::UserRole, true); //listWidget->count() > 0);
    listWidget->addItem(wdgItem);
}

void ChatAccounts::removeAccount()
{
    QListWidgetItem *item = listWidget->item(listWidget->currentRow());
    if (item && item->data(Qt::UserRole).toBool())
    {
        listWidget->takeItem(listWidget->currentRow());
        updateButtons();
    }
}

void ChatAccounts::setAccounts(const QStringList &accounts)
{
    listWidget->clear();
    foreach (const QString &jid, accounts)
        addEntry(jid);
}

void ChatAccounts::updateButtons()
{
    QListWidgetItem *item = listWidget->currentItem();
    if (item)
        removeButton->setEnabled(item->data(Qt::UserRole).toBool());
    else
        removeButton->setEnabled(false);
}

void ChatAccounts::validate()
{
    accept();
}

