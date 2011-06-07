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

    signal locationChanged(variant location)

    height: 24

    function push(crumb) {
        crumbs.append(crumb);
        crumbBar.locationChanged(crumbs.get(crumbs.count - 1));
    }

    function pop() {
        if (crumbs.count > 1) {
            crumbs.remove(crumbs.count - 1);
            crumbBar.locationChanged(crumbs.get(crumbs.count - 1));
        }
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

            delegate: Item {
                property bool isLast: model.index == crumbs.count - 1

                height: row.height
                width: crumb.width + row.spacing + separator.width

                Rectangle {
                    id: crumb

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
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
                        enabled: !isLast
                        hoverEnabled: true

                        onEntered: {
                            parent.state = 'hovered'
                        }

                        onExited: {
                            parent.state = ''
                        }

                        onClicked: {
                            if (mouse.button == Qt.LeftButton) {
                                // remove crumbs below current location
                                for (var i = crumbs.count - 1; i > model.index; i--) {
                                    crumbs.remove(i);
                                }
                                crumbBar.locationChanged(model);
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

                    anchors.left: crumb.right
                    anchors.leftMargin: row.spacing
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    font.bold: true
                    opacity: isLast ? 0 : 1
                    text: '&rsaquo;'
                    textFormat: Text.RichText
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
 
