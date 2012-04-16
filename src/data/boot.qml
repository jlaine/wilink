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

    property QtObject addWindow

    AccountModel {
        id: accountModel
    }

    ListHelper {
        id: accountHelper

        model: accountModel
    }

    Repeater {
        model: accountModel
        delegate: Rectangle {
            id: item

            property QtObject wnd

            height: 30
            width: 30
            color: 'green'

            Component.onCompleted: {
                wnd = window.createWindow();
                wnd.objectName = jid;

                if (application.isMobile) {
                    wnd.fullScreen = !application.isAndroid;
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
                wnd.source = application.isMeego ? 'MeegoMain.qml' : 'MainWindow.qml';
                wnd.showAndRaise();
            }
            Component.onDestruction: {
                console.log("del wnd");
                window.destroyWindow(wnd);
            }
        }
    }

    Component.onCompleted: {
        application.resetWindows.connect(function() {
            // check we have a valid account
            if (!accountHelper.count) {
                console.log("no accounts");
                if (!Qt.isQtObject(addWindow))
                    addWindow = window.createWindow();
                addWindow.source = application.isMeego ? 'MeegoSetup.qml' : 'SetupWindow.qml';
                //const QSize size = QApplication::desktop()->availableGeometry(window).size();
                //window.move((size.width() - window->width()) / 2, (size.height() - window->height()) / 2);
                addWindow.showAndRaise();
                return;
            } else if (Qt.isQtObject(addWindow)) {
                window.destroyWindow(addWindow);
                addWindow = null;
            }
        });
    }
}
