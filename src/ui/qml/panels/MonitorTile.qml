import QtQuick
import QtQuick.Layouts

// One Serving monitor tile (K2 card): a caption, a mono value, an optional second line and an
// optional bar (a tier color for memory).
Card {
    id: root

    property string title
    property string value
    property string detail
    property real fraction: -1
    property color barColor: Theme.accent

    implicitWidth: Theme.monitorTileMinWidth

    Text {
        text: root.title
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.text2
        elide: Text.ElideRight
        Layout.fillWidth: true
    }

    Text {
        text: root.value
        font.family: Theme.monoFamily
        font.pointSize: Theme.titleSize
        fontSizeMode: Text.HorizontalFit
        minimumPointSize: Theme.captionSize
        color: Theme.text1
        Layout.fillWidth: true
    }

    UsageBar {
        fraction: Math.max(0, root.fraction)
        fillColor: root.barColor
        visible: root.fraction >= 0
        Layout.fillWidth: true
    }

    Text {
        text: root.detail
        visible: root.detail !== ""
        font.family: Theme.monoFamily
        font.pointSize: Theme.captionSize
        color: Theme.text3
        elide: Text.ElideRight
        Layout.fillWidth: true
    }

    Item {
        Layout.fillHeight: true
    }
}
