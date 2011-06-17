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

#ifndef __WILINK_SHARES_OPTIONS_H__
#define __WILINK_SHARES_OPTIONS_H__

#include "plugins/preferences.h"

class ShareFolderModel;
class SharePlaceModel;
class QLineEdit;
class QTreeView;

/** View for displaying a tree of share items.
 */
class ShareOptions : public ChatPreferencesTab
{
    Q_OBJECT

public:
    ShareOptions();
    bool save();

protected:
    void showEvent(QShowEvent *event);

private slots:
    void browse();
    void directorySelected(const QString &path);
    void fewerFolders();
    void moreFolders();
    void scrollToHome();

private:
    QPushButton *m_moreButton;
    QPushButton *m_fewerButton;
    QLineEdit *m_directoryEdit;
    SharePlaceModel *m_placesModel;
    QTreeView *m_placesView;
    ShareFolderModel *m_fsModel;
    QTreeView *m_fsView;
};

#endif
