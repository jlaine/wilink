import QtQuick 1.0

Rectangle {
    width: 180; height: 200

    Component {
        id: playerDelegate
        Item {
            width: 180; height: 40
            Row {
                Column {
                    width: 100
                    Text { text: '<b>' + artist + '</b>' }
                    Text { text: title }
                }
                Text {

                    function padNumber(n) {
                        var s = n.toString();
                        if (s.length == 1)
                            return '0' + s;
                        return s;
                    }
                    function formatDuration(ms) {
                        var secs = ms / 1000;
                        var hours = Number(secs / 3600).toFixed();
                        var minutes = Number(secs / 60).toFixed() % 3600;
                        var seconds = Number(secs).toFixed() % 60

                        if (hours > 0)
                            return hours.toString() + ':' + padNumber(minutes) + ':' + padNumber(seconds);
                        else
                            return minutes.toString() + ':' + padNumber(seconds);
                    }
                    text: formatDuration(duration)
                }
            }
        }
    }

    ListView {
        anchors.fill: parent
        model: PlayerModel {}
        delegate: playerDelegate
        highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
        focus: true
    }
}
