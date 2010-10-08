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

#ifndef __WILINK_CHAT_SEARCH_H__
#define __WILINK_CHAT_SEARCH_H__

#include <QTextDocument>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;

class ChatSearchBar : public QWidget
{
    Q_OBJECT

public:
    ChatSearchBar(QWidget *parent = 0);
    void setControlsVisible(bool visible);
    void setDelay(int msecs);
    void setText(const QString &text);

signals:
    void find(const QString &search, QTextDocument::FindFlags flags, bool changed);
    void findClear();
    void displayed(bool visible);

public slots:
    void activate();
    void deactivate();
    void findFinished(bool found);
    void findNext();
    void findPrevious();

private slots:
    void slotSearchDelayed();
    void slotSearchChanged();

private:
    QTimer *findTimer;
    QLabel *findPrompt;
    QLineEdit *findBox;
    QCheckBox *m_findCase;
    QPushButton *m_findPrevious;
    QPushButton *m_findNext;
    QPushButton *m_findClose;
    QPalette normalPalette;
    QPalette failedPalette;
};

#endif
