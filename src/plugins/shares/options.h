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

#ifndef __WILINK_SHARES_OPTIONS_H__
#define __WILINK_SHARES_OPTIONS_H__

#include <QDialog>

class FoldersModel;
class QLineEdit;
class QListWidget;
class QTreeView;
class QXmppShareDatabase;

/** View for displaying a tree of share items.
 */
class ChatSharesOptions : public QDialog
{
    Q_OBJECT

public:
    ChatSharesOptions(QXmppShareDatabase *database, QWidget *parent = 0);

public slots:
    void show();

private slots:
    void browse();
    void directorySelected(const QString &path);
    void scrollToHome();
    void validate();

private:
    QXmppShareDatabase *m_database;
    QLineEdit *m_directoryEdit;
    QListWidget *m_placesView;
    FoldersModel *m_fsModel;
    QTreeView *m_fsView;
};

#endif
