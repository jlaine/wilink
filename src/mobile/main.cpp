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

#include <QTimer>

#include "application.h"

int main(int argc, char *argv[])
{
    /* Create application */
    Application::platformInit();
    Application app(argc, argv);

#if 0
    /* Show main window */
    MainWindow window;
    window.open("qxmpp.test1@gmail.com", "qxmpp123");
#ifdef Q_OS_SYMBIAN
    window.showMaximized();
#else
    window.resize(200, 320);
    window.show();
#endif
#endif

    /* Show chat windows */
    QTimer::singleShot(0, &app, SLOT(resetChats()));

    /* Run application */
    return app.exec();
}

