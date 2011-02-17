import QtQuick 1.0

Rectangle {
    width: 320
    height: 400

    Component {
        id: playerDelegate
        Item {
            id: item
            height: 40
            width: parent.width

            function formatDuration(ms) {
                var secs = ms / 1000;
                var hours = Number(secs / 3600).toFixed();
                var minutes = Number(secs / 60).toFixed() % 3600;
                var seconds = Number(secs).toFixed() % 60

                function padNumber(n) {
                    var s = n.toString();
                    if (s.length == 1)
                        return '0' + s;
                    return s;
                }

                if (hours > 0)
                    return hours.toString() + ':' + padNumber(minutes) + ':' + padNumber(seconds);
                else
                    return minutes.toString() + ':' + padNumber(seconds);
            }

            Rectangle {
                Column {
                    id: firstColumn
                    x: 10
                    width: item.width - 60
                    Text { text: '<b>' + artist + '</b>' }
                    Text { text: title }
                }
                Text {
                    anchors.left: firstColumn.right
                    text: formatDuration(duration)
                }
            }
        }
    }

    ListView {
        id: playerView
        anchors.fill: parent
        model: playerModel
        delegate: playerDelegate
        highlight: Rectangle { color: "lightsteelblue"; radius: 5; width: playerView.width }
        focus: true
    }
}
