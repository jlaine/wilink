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
import 'utils.js' as Utils

Panel {
    id: newsPanel

    property bool singlePanel: width < 500

    ContactView {
        id: sidebar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        title: qsTr('My news')
        width: newsPanel.singlePanel ? parent.width : 200

        delegate: Item {
            id: item

            height: appStyle.icon.normalSize
            width: view.width - 1

            Image {
                id: image

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: appStyle.spacing.horizontal
                height: appStyle.icon.normalSize
                width: appStyle.icon.normalSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: model.imageSource
            }

            Label {
                id: titleLabel

                anchors.verticalCenter: image.verticalCenter
                anchors.left: image.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                anchors.rightMargin: appStyle.spacing.horizontal
                elide: Text.ElideRight
                font.bold: true
                text: model.title
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    sidebar.currentIndex = model.index;
                    mainView.model.source = model.link;
                    if (newsPanel.singlePanel)
                        newsPanel.state = 'no-sidebar';
                }
            }
        }

        model: ListModel {
            ListElement {
                title: 'BBC News - World'
                imageSource: 'rss.png'
                link: 'http://feeds.bbci.co.uk/news/world/rss.xml'
            }

            ListElement {
                title: 'BBC News - Technology'
                imageSource: 'rss.png'
                link: 'http://feeds.bbci.co.uk/news/technology/rss.xml'
            }
        }
    }

    Item {
        id: main

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: sidebar.right
        anchors.right: parent.right
        visible: width > 0

        PanelHeader {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            iconSource: 'rss.png'
            title: qsTr('News reader')
        }

        ListView {
            id: mainView

            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: scrollBar.left
            anchors.margins: 2
            spacing: appStyle.spacing.horizontal
            focus: true
            delegate: Item {
                id: item

                height: appStyle.icon.normalSize
                width: mainView.width - 1

                Image {
                    id: image

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.spacing.horizontal
                    height: appStyle.icon.normalSize
                    width: appStyle.icon.normalSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    source: model.imageSource
                }

                Label {
                    id: titleLabel

                    anchors.verticalCenter: image.verticalCenter
                    anchors.left: image.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.spacing.horizontal
                    elide: Text.ElideRight
                    font.bold: true
                    text: model.title
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: mainView.currentIndex = model.index
                }

                Label {
                    id: descriptionLabel

                    anchors.top: image.bottom
                    anchors.topMargin: appStyle.spacing.vertical
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.spacing.horizontal
                    visible: false
                    wrapMode: Text.WordWrap

                    onLinkActivated: Qt.openUrlExternally(link)
                }

                states: State {
                    name: 'details'
                    when: mainView.currentIndex == model.index

                    PropertyChanges {
                        target: descriptionLabel
                        text: '<p>' + model.description + '</p><p><a href="' + model.link + '">' + model.link + '</a></p>'
                        visible: true
                    }

                    PropertyChanges {
                        target: item
                        height: image.height + 2*appStyle.spacing.vertical + descriptionLabel.height
                    }
                }
            }

            highlight: Highlight {
                width: mainView.width
            }
            highlightMoveDuration: appStyle.highlightMoveDuration

            model: XmlListModel {
                id: newsModel

                query: '/rss/channel/item'

                XmlRole { name: 'description'; query: 'description/string()' }
                XmlRole { name: 'link'; query: 'link/string()' }
                XmlRole { name: 'pubDate'; query: 'pubDate/string()' }
                XmlRole { name: 'imageSource'; query: '*:thumbnail[1]/@url/string()' }
                XmlRole { name: 'title'; query: 'title/string()' }
            }
        }

        ScrollBar {
            id: scrollBar

            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            flickableItem: mainView
        }
    }

    states: State {
        name: 'no-sidebar'

        PropertyChanges { target: sidebar; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: sidebar; properties: 'width'; duration: appStyle.animation.normalDuration }
    }

    Component.onCompleted: {
        mainView.model.source = sidebar.model.get(0).link;
    }
}

