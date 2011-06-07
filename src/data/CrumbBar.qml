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

import QtQuick 1.0
import wiLink 1.2

Item {
    id: crumbBar

    property alias model: crumbView.model
    property Item view

    height: 24

    function goBack() {
        var crumb = crumbs.get(crumbs.count - 1);
        view.model.rootJid = crumb.jid;
        view.model.rootNode = crumb.node;
        view.model.rootName = crumb.name;
        crumbs.remove(crumbs.count - 1);
    }

    function goTo(crumb) {
        if (view.model.rootJid == crumb.jid &&
            view.model.rootNode == crumb.node) {
            return;
        }

        crumbBar.model.append({'name': view.model.rootName, 'jid': view.model.rootJid, 'node': view.model.rootNode});
        view.model.rootJid = crumb.jid;
        view.model.rootNode = crumb.node;
        view.model.rootName = crumb.name;
    }

    Row {
        id: row
        anchors.left: parent.left
        anchors.top:  parent.top
        anchors.bottom:  parent.bottom
        spacing: 4

        Repeater {
            id: crumbView

            model: ListModel {
                id: crumbs
            }

            delegate: Row {
                spacing: 4

                width: name.width + separator.width

                Rectangle {
                    height: row.height
                    width: name.width

                    Text {
                        id: name

                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        color: '#000000'
                        text: model.name
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        anchors.fill: parent
                        hoverEnabled: true

                        onEntered: {
                            parent.state = 'hovered'
                        }

                        onExited: {
                            parent.state = ''
                        }

                        onClicked: {
                            if (mouse.button == Qt.LeftButton) {
                                console.log("clicked: " + model.name);
                                var row = -1;
                                for (var i = 0; i < crumbs.count; i++) {
                                    var obj = crumbs.get(i);
                                    if (obj.jid == model.jid && obj.node == model.node) {
                                        row = i;
                                        break;
                                    }
                                }
                                if (row < 0) {
                                    console.log("unknown row clicked");
                                    return;
                                }

                                // navigate to the specified location
                                view.model.rootJid = model.jid;
                                view.model.rootNode = model.node;
                                view.model.rootName = model.name;

                                // remove crumbs below current location
                                for (var i = crumbs.count - 1; i >= row; i--) {
                                    crumbs.remove(i);
                                }
                            }
                        }
                    }

                    states: State {
                        name: 'hovered'
                        PropertyChanges {
                            target: name
                            color: '#aaaaaa'
                        }
                    }

                }

                Text {
                    id: separator
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: name.right
                    font.bold: true
                    text: '&rsaquo;'
                    textFormat: Text.RichText
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            color: '#000000'
            text: view.model.rootName
            verticalAlignment: Text.AlignVCenter
        }
    }
}
 
