import QtQuick 1.1

GridView {
    id: root
    width: 600; height: 400
    cellHeight: 240
    cellWidth: 240

    property int imageSize: 200

    model: XmlListModel {
        id: listModel
        query: '/rss/channel/item'
        source: 'http://api.flickr.com/services/feeds/groups_pool.gne?id=863004@N20&lang=fr-fr&format=rss2'

        XmlRole { name: 'source'; query: 'enclosure[@type="image/jpeg"]/@url/string()' }
        XmlRole { name: 'title'; query: 'title/string()' }
    }

    delegate: Item {
        id: item
        property int animDuration: 0

        Image {
            parent: root
            x: item.x + 15
            y: item.y

            fillMode: Image.PreserveAspectFit
            width: 200
            height: 200
            smooth: true
            source: model.source
            transform: Rotation {
                id: rotation
                Behavior on angle {
                    NumberAnimation { duration: item.animDuration }
                }
            }

            Behavior on x {
                NumberAnimation { duration: item.animDuration }
            }

            Behavior on y {
                NumberAnimation { duration: item.animDuration }
            }

            Timer {
                interval: 100
                repeat: false
                running: true
                onTriggered: {
                    item.animDuration = 400;
                    rotation.angle = (Math.random() - 0.5) * 10;
                }
            }
            Component.onCompleted: {
            }
        }
    }

    Timer {
        id: delay

        interval: 500
        repeat: false

        onTriggered: {
            console.log("randomize!!");
            for (var i = 0; i < repeater.count; i++) {
                repeater.itemAt(i).randomizePosition();
            }
        }
    }
}

