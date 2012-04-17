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

import QtQuick 1.1
import wiLink 2.0

Rectangle {
    id: boot

    AccountModel {
        id: accountModel
    }

    Component.onCompleted: {

        application.resetWindows.connect(function() {
            // check we have a valid account
            if (accountModel.count) {
                window.minimumWidth = 240;
                window.minimumHeight = 360;

                if (application.isMobile) {
                    window.fullScreen = !application.isAndroid;
                } else {
                    // restore window geometry
    /*
                    var xpos = 30;
                    const QByteArray geometry = d->appSettings->windowGeometry(jid);
                    if (!geometry.isEmpty()) {
                        window->restoreGeometry(geometry);
                        window->setFullScreen(false);
                    } else {
                        QSize size = QApplication::desktop()->availableGeometry(window).size();
                        size.setHeight(size.height() - 100);
                        size.setWidth((size.height() * 4.0) / 3.0);
                        window->resize(size);
                        window->move(xpos, ypos);
                    }
                    xpos += 100;
    */
                }
                loader.source = 'MainWindow.qml';
            } else {
                window.minimumWidth = 360;
                window.minimumHeight = 240;
                window.size = window.minimumSize;
                //const QSize size = QApplication::desktop()->availableGeometry(window).size();
                //window.move((size.width() - window->width()) / 2, (size.height() - window->height()) / 2);
                loader.source = 'SetupWindow.qml';
            }

            window.showAndRaise();
        });

        application.showWindows.connect(function() {
            window.showAndRaise();
        });
    }

    Loader {
        id: loader
        anchors.fill: parent
    }
}
