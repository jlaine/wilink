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

#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>

#include "QXmppUtils.h"

#include "application.h"
#include "accounts.h"

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


