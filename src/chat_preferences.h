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

#ifndef __WILINK_CHAT_PREFERENCES_H__
#define __WILINK_CHAT_PREFERENCES_H__

#include <QDialog>

class ChatPreferencesPrivate;
class ChatPreferencesTab;

/** ChatPreferences is the base class for all settings tabs.
 */
class ChatPreferences : public QDialog
{
    Q_OBJECT

public:
    ChatPreferences(QWidget *parent = 0);
    ~ChatPreferences();

    void addTab(ChatPreferencesTab *tab);

private slots:
    void validate();

private:
    ChatPreferencesPrivate * const d;
};

class ChatPreferencesTab : public QWidget
{
    Q_OBJECT

public:
    ChatPreferencesTab();
    virtual bool save() { return true; };
};

#endif
