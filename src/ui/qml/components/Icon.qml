import QtQuick
import QtQuick.Effects

// Lucide icon tinted to any Theme color (01 section 6.7). The SVGs are stored with white
// strokes so colorization maps them straight onto `color`.
Item {
    id: root

    property string name
    property int size: Theme.iconSize
    property color color: Theme.text2

    implicitWidth: size
    implicitHeight: size

    Image {
        id: glyph
        anchors.fill: parent
        source: root.name ? "qrc:/qt/qml/GigaStrata/icons/" + root.name + ".svg" : ""
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        visible: false
    }

    MultiEffect {
        anchors.fill: glyph
        source: glyph
        colorization: 1.0
        colorizationColor: root.color
    }
}
