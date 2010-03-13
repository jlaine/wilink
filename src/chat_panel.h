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

#ifndef __WDESKTOP_CHAT_PANEL_H__
#define __WDESKTOP_CHAT_PANEL_H__

#include <QWidget>

class QLabel;
class QPushButton;

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    ChatPanel(QWidget *parent);
    void setWindowIcon(const QIcon &icon);
    void setWindowExtra(const QString &extra);
    void setWindowTitle(const QString &title);

protected:
    QLayout *statusBar();

signals:
    void closeTab();
    void notifyTab();
    void showTab();

private:
    QPushButton *closeButton;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QString windowExtra;
};

#endif
