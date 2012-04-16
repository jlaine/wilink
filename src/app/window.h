/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __WILINK_WINDOW_H__
#define __WILINK_WINDOW_H__

#include <QMainWindow>
#include <QUrl>

class QFileDialog;
class WindowPrivate;

/** Chat represents the user interface's main window.
 */
class Window : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(bool isActiveWindow READ isActiveWindow NOTIFY windowStateChanged)
    Q_PROPERTY(bool fullScreen READ isFullScreen WRITE setFullScreen NOTIFY windowStateChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)

public:
    Window(QWidget *parent = 0);
    ~Window();
    void setFullScreen(bool fullScreen);

    QUrl source() const;
    void setSource(const QUrl &source);

signals:
    void showAbout();
    void showHelp();
    void showPreferences();
    void sourceChanged();
    void windowStateChanged();

public slots:
    void alert();
    QFileDialog *fileDialog();
    void showAndRaise();

private slots:
    void _q_statusChanged();

protected:
    void changeEvent(QEvent *event);

private:
    WindowPrivate *d;
};

#endif
