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
#include <QAuthenticator>
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
#include "chat_utils.h"

static const char *accountsKey = "ChatAccounts";

/** Returns the authentication realm for the given JID.
 */
static QString authRealm(const QString &jid)
{
    const QString domain = jidToDomain(jid);
    if (domain == "wifirst.net")
        return QLatin1String("www.wifirst.net");
    else if (domain == "gmail.com")
        return QLatin1String("www.google.com");
    return domain;
}

ChatAccountPrompt::ChatAccountPrompt(QWidget *parent)
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

    layout->addWidget(new QLabel("<b>" + tr("Password") + "</b>"), 2, 0);
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit, 2, 1);

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    layout->addWidget(m_statusLabel, 3, 0, 1, 2);

    QLabel *helpLabel = new QLabel("<i>" + tr("If you need help, please refer to the <a href=\"%1\">%2 FAQ</a>.")
        .arg(QLatin1String(HELP_URL), qApp->applicationName()) + "</i>");
    helpLabel->setOpenExternalLinks(true);
#ifdef WILINK_EMBEDDED
    helpLabel->setWordWrap(true);
#endif
    layout->addWidget(helpLabel, 4, 0, 1, 2);

    m_buttonBox = new QDialogButtonBox;
    m_buttonBox->addButton(QDialogButtonBox::Ok);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(m_buttonBox, 5, 0, 1, 2);

    setDomain(QString());
    setLayout(layout);
    setWindowTitle(tr("Add an account"));

    /* connect signals */
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(testAccount()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_testClient, SIGNAL(connected()), this, SLOT(accept()));
    connect(m_testClient, SIGNAL(disconnected()), this, SLOT(testFailed()));
}

QString ChatAccountPrompt::jid() const
{
    QString jid = m_jidEdit->text();
    if (!m_domain.isEmpty())
        jid = jid.split('@').first() + "@" + m_domain;
    return jid;
}

QString ChatAccountPrompt::password() const
{
    return m_passwordEdit->text();
}

void ChatAccountPrompt::setAccounts(const QStringList &accounts)
{
    m_accounts = accounts;
}

void ChatAccountPrompt::setDomain(const QString &domain)
{
    m_domain = domain;
    if (m_domain.isEmpty())
    {
        m_jidLabel->setText("<b>" + tr("Address") + "</b>");
        m_promptLabel->setText(tr("Enter the address and password for the account you want to add."));
    }
    else
    {
        m_jidLabel->setText("<b>" + tr("Username") + "</b>");
        m_promptLabel->setText(tr("Enter the username and password for your '%1' account.").arg(m_domain));
    }
}

void ChatAccountPrompt::showMessage(const QString &message, bool isError)
{
    m_statusLabel->setStyleSheet(isError ? m_errorStyle : QString());
    m_statusLabel->setText(message);
    m_statusLabel->show();
    resize(size().expandedTo(sizeHint()));
}

void ChatAccountPrompt::testAccount()
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
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    showMessage(tr("Checking your username and password.."), false);
    QXmppConfiguration config;
    config.setJid(jid());
    config.setPassword(password());
    config.setResource("AccountCheck");
    m_testClient->connectToServer(config);
}

void ChatAccountPrompt::testFailed()
{
    showMessage(tr("Could not connect, please check your username and password."), true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

ChatPasswordPrompt::ChatPasswordPrompt(const QString &jid, QWidget *parent)
    : QDialog(parent)
{
    QGridLayout *layout = new QGridLayout;

    QLabel *promptLabel = new QLabel;
    promptLabel->setText(tr("Enter the password for your '%1' account.").arg(jidToDomain(jid)));
    promptLabel->setWordWrap(true);
    layout->addWidget(promptLabel, 0, 0, 1, 2);

    layout->addWidget(new QLabel("<b>" + tr("Address") + "</b>"), 1, 0);
    QLineEdit *usernameEdit = new QLineEdit;
    usernameEdit->setText(jid);
    usernameEdit->setEnabled(false);
    layout->addWidget(usernameEdit, 1, 1);

    layout->addWidget(new QLabel("<b>" + tr("Password") + "</b>"), 2, 0);
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit, 2, 1);

    QLabel *helpLabel = new QLabel("<i>" + tr("If you need help, please refer to the <a href=\"%1\">%2 FAQ</a>.")
        .arg(QLatin1String(HELP_URL), qApp->applicationName()) + "</i>");
    helpLabel->setOpenExternalLinks(true);
    layout->addWidget(helpLabel, 3, 0, 1, 2);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox, 4, 0, 1, 2);

    setLayout(layout);
    setWindowTitle(tr("Password required"));
}

QString ChatPasswordPrompt::password() const
{
    return m_passwordEdit->text();
}

ChatAccounts::ChatAccounts(QWidget *parent)
    : QDialog(parent),
    m_changed(false)
{
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *helpLabel = new QLabel(
        tr("In addition to your %1 account, %2 can connect to additional chat accounts such as Google Talk and Facebook.")
            .arg(qApp->organizationName())
            .arg(qApp->applicationName()));
    helpLabel->setWordWrap(true);
    layout->addWidget(helpLabel);

    m_listWidget = new QListWidget;
    m_listWidget->setIconSize(QSize(32, 32));
    connect(m_listWidget, SIGNAL(currentRowChanged(int)),
            this, SLOT(updateButtons()));
    layout->addWidget(m_listWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addAccount()));
    buttonBox->addButton(addButton, QDialogButtonBox::ActionRole);

    m_removeButton = new QPushButton;
    m_removeButton->setIcon(QIcon(":/remove.png"));
    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(removeAccount()));
    buttonBox->addButton(m_removeButton, QDialogButtonBox::ActionRole);

    layout->addWidget(buttonBox);

    setLayout(layout);

    /* load accounts */
    m_settings = new QSettings(this);
    const QStringList accountJids = accounts();
    foreach (const QString &jid, accountJids)
        m_listWidget->addItem(new QListWidgetItem(QIcon(":/chat.png"), jid));
    updateButtons();
}

QStringList ChatAccounts::accounts() const
{
    return m_settings->value(accountsKey).toStringList();
}

bool ChatAccounts::addAccount(const QString &domain)
{
    ChatAccountPrompt dlg(this);
    dlg.setAccounts(accounts());
    dlg.setDomain(domain);
    if (dlg.exec())
    {
        m_changed = true;
        const QString jid = dlg.jid();
        QNetIO::Wallet::instance()->setCredentials(authRealm(jid), jid, dlg.password());
        m_settings->setValue(accountsKey, accounts() << jid);
        m_listWidget->addItem(new QListWidgetItem(QIcon(":/chat.png"), jid));
        return true;
    }
    return false;
}

/** Check we have a valid account.
 */
void ChatAccounts::check()
{
    const QString requiredDomain("wifirst.net");

    bool foundAccount = false;
    const QStringList accountJids = accounts();
    foreach (const QString &jid, accountJids)
    {
        if (!isBareJid(jid))
        {
            qWarning() << "Removing bad account" << jid;
            removeAccount(jid);
        }
        else if (jidToDomain(jid) == requiredDomain)
            foundAccount = true;
    }

    if (!foundAccount && !addAccount(requiredDomain))
    {
        qApp->quit();
        return;
    }
}

bool ChatAccounts::changed() const
{
    return m_changed;
}

bool ChatAccounts::getPassword(const QString &jid, QString &password, QWidget *parent)
{
    const QString realm = authRealm(jid);
    if (!QNetIO::Wallet::instance())
    {
        qWarning("No wallet!");
        return false;
    }

    /* check if we have a stored password that differs from the given one */
    QString tmpJid(jid), tmpPassword(password);
    if (QNetIO::Wallet::instance()->getCredentials(realm, tmpJid, tmpPassword) &&
        tmpJid == jid && tmpPassword != password)
    {
        password = tmpPassword;
        return true;
    }

    /* prompt user */
    ChatPasswordPrompt dialog(jid, parent);
    while (dialog.password().isEmpty())
    {
        if (dialog.exec() != QDialog::Accepted)
            return false;
    }

    /* store new password */
    password = dialog.password();
    QNetIO::Wallet::instance()->setCredentials(realm, jid, password);
    return true;
}

void ChatAccounts::removeAccount()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (item)
        removeAccount(item->text());
}

void ChatAccounts::removeAccount(const QString &jid)
{
    m_changed = true;

    // remove credentials
    const QString realm = authRealm(jid);
    qDebug() << "Removing credentials for" << realm;
    QNetIO::Wallet::instance()->deleteCredentials(realm);

    // remove account
    QStringList accounts = m_settings->value(accountsKey).toStringList();
    accounts.removeAll(jid);
    m_settings->setValue(accountsKey, accounts);

    // update buttons
    QList<QListWidgetItem*> goners = m_listWidget->findItems(jid, Qt::MatchExactly);
    foreach (QListWidgetItem *item, goners)
    {
        int row = m_listWidget->row(item);
        delete m_listWidget->takeItem(row);
    }
    updateButtons();
}

void ChatAccounts::updateButtons()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    m_removeButton->setEnabled(item != 0);
}

