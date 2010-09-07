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

#ifndef __WILINK_CHAT_ACCOUNTS_H__
#define __WILINK_CHAT_ACCOUNTS_H__

#include <QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QXmppClient;

class AddChatAccount : public QDialog
{
    Q_OBJECT

public:
    AddChatAccount(QWidget *parent = 0);

    QString jid() const;
    QString password() const;

    void setDomain(const QString &domain);

private slots:
    void testAccount();
    void testFailed();

private:
    QString m_domain;
    QLineEdit *m_jidEdit;
    QLineEdit *m_passwordEdit;
    QLabel *m_promptLabel;
    QLabel *m_statusLabel;
    QXmppClient *m_testClient;
};

class ChatAccounts : public QDialog
{
    Q_OBJECT

public:
    ChatAccounts(QWidget *parent = 0);
    QStringList accounts() const;
    void setAccounts(const QStringList &accounts);

private slots:
    void addAccount();
    void removeAccount();
    void updateButtons();
    void validate();

private:
    void addEntry(const QString &jid);

    QListWidget *listWidget;
    QPushButton *removeButton;
};

#endif
