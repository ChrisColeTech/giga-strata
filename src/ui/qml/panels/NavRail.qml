pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

// The rail of the two screens (04 section 1): Models and Serving.
Rectangle {
    id: root

    property int currentIndex: 0

    implicitWidth: Theme.railWidth
    color: Theme.surface1

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.topMargin: Theme.space2
        width: parent.width
        spacing: Theme.space1

        Repeater {
            model: [
                { label: qsTr("Models"), icon: "layers" },
                { label: qsTr("Serving"), icon: "server" }
            ]
            delegate: NavRailItem {
                required property var modelData
                required property int index
                text: modelData.label
                iconName: modelData.icon
                isActive: root.currentIndex === index
                onClicked: root.currentIndex = index
            }
        }
    }
}
