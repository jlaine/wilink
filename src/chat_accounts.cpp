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
#include <QSettings>

#include "QXmppClient.h"
#include "QXmppUtils.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "chat_accounts.h"
#include "utils.h"

static const char *accountsKey = "ChatAccounts";

AddChatAccount::AddChatAccount(QWidget *parent)
    : QDialog(parent),
    m_errorStyle("QLabel { color: red; }")
{
    m_testClient = new QXmppClient(this);

    QGridLayout *layout = new QGridLayout;

    m_promptLabel = new QLabel;
    m_promptLabel->setWordWrap(true);
    layout->addWidget(m_promptLabel, 0, 0, 1, 2);

    m_jidLabel = new QLabel;
    layout->addWidget(m_jidLabel, 1, 0);
    m_jidEdit = new QLineEdit;
    layout->addWidget(m_jidEdit, 1, 1);

    layout->addWidget(new QLabel(tr("Password")), 2, 0);
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit, 2, 1);

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    layout->addWidget(m_statusLabel, 3, 0, 1, 2);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox, 4, 0, 1, 2);

    setDomain(QString());
    setLayout(layout);
    setWindowTitle(tr("Add an account"));

    /* connect signals */
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(testAccount()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_testClient, SIGNAL(connected()), this, SLOT(testSucceeded()));
    connect(m_testClient, SIGNAL(disconnected()), this, SLOT(testFailed()));
}

QString AddChatAccount::jid() const
{
    QString jid = m_jidEdit->text();
    if (!m_domain.isEmpty())
        jid = jid.split('@').first() + "@" + m_domain;
    return jid;
}

QString AddChatAccount::password() const
{
    return m_passwordEdit->text();
}

void AddChatAccount::setAccounts(const QStringList &accounts)
{
    m_accounts = accounts;
}

void AddChatAccount::setDomain(const QString &domain)
{
    m_domain = domain;
    if (m_domain.isEmpty())
    {
        m_jidLabel->setText(tr("Address"));
        m_promptLabel->setText(tr("Enter the address and password for the account you want to add."));
    }
    else
    {
        m_jidLabel->setText(tr("Username"));
        m_promptLabel->setText(tr("Enter the username and password for your '%1' account.").arg(m_domain));
    }
}

void AddChatAccount::showMessage(const QString &message, bool isError)
{
    m_statusLabel->setStyleSheet(isError ? m_errorStyle : QString());
    m_statusLabel->setText(message);
    m_statusLabel->show();
    resize(size().expandedTo(sizeHint()));
}

void AddChatAccount::testAccount()
{
    // normalise input
    m_jidEdit->setText(m_jidEdit->text().trimmed().toLower());
    if (m_jidEdit->text().isEmpty() || m_passwordEdit->text().isEmpty())
        return;

    // check JID is valid
    if (!isBareJid(jid()))
    {
        showMessage(tr("The address you entered is invalid."), true);
        return;
    }

    // check we don't already have an account for this domain
    const QString domain = jidToDomain(jid());
    foreach (const QString &account, m_accounts)
    {
        if (jidToDomain(account) == domain)
        {
            showMessage(tr("You already have an account for '%1'.").arg(domain), true);
            return;
        }
    }

    // test account
    showMessage(tr("Checking your username and password.."), false);
    QXmppConfiguration config;
    config.setJid(jid());
    config.setPassword(password());
    config.setResource("AccountCheck");
    m_testClient->connectToServer(config);
}

void AddChatAccount::testFailed()
{
    showMessage(tr("Could not connect, please check your username and password."), true);
}

void AddChatAccount::testSucceeded()
{
    // store credentials
    QNetIO::Wallet::instance()->setCredentials(Application::authRealm(jid()),
        jid(), password());

    accept();
}

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

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
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

    /* load accounts */
    m_settings = new QSettings(this);
    const QStringList accounts = m_settings->value(accountsKey).toStringList();
    foreach (const QString &jid, accounts)
        addEntry(jid);
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
    AddChatAccount dlg;
    dlg.setAccounts(accounts());
    if (dlg.exec())
    {
        // add account
        QStringList accounts = m_settings->value(accountsKey).toStringList();
        accounts << dlg.jid();
        m_settings->setValue(accountsKey, accounts);

        addEntry(dlg.jid());
    }
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
    QListWidgetItem *item = listWidget->takeItem(listWidget->currentRow());
    if (!item)
        return;
    const QString jid = item->text();

    // remove credentials
    const QString realm = Application::authRealm(jid);
    qDebug() << "Removing credentials for" << realm;
    QNetIO::Wallet::instance()->deleteCredentials(realm);

    // remove account
    QStringList accounts = m_settings->value(accountsKey).toStringList();
    accounts.removeAll(jid);
    m_settings->setValue(accountsKey, accounts);

    // update buttons
    delete item;
    updateButtons();
}

void ChatAccounts::updateButtons()
{
    QListWidgetItem *item = listWidget->currentItem();
    if (item)
        removeButton->setEnabled(item->data(Qt::UserRole).toBool());
    else
        removeButton->setEnabled(false);
}

