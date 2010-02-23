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

#ifndef __WDESKTOP_CHAT_ACCOUNTS_H__
#define __WDESKTOP_CHAT_ACCOUNTS_H__

#include <QDialog>

class QListWidget;
class QListWidgetItem;

class ChatAccounts : public QDialog
{
    Q_OBJECT

public:
    ChatAccounts(QWidget *parent = NULL);
    QStringList accounts() const;
    void setAccounts(const QStringList &accounts);

private slots:
    void addAccount();
    void itemClicked(QListWidgetItem *item);
    void removeAccount();
    void validate();

private:
    void addEntry(const QString &jid);

    QListWidget *listWidget;
    QPushButton *removeButton;
};

#endif
