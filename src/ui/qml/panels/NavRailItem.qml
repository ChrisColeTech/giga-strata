import QtQuick
import QtQuick.Controls.Basic

// One rail entry (N2): icon over caption, accent bar on the left when active.
AbstractButton {
    id: root

    property string iconName
    property bool isActive: false

    implicitWidth: Theme.railWidth
    implicitHeight: Theme.railItemHeight
    focusPolicy: Qt.StrongFocus
    Accessible.name: text

    background: Item {
        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.railIndicatorWidth
            height: parent.height - Theme.space3
            radius: Theme.radiusLg
            color: Theme.accent
            opacity: root.isActive ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.space1
            radius: Theme.radiusMd
            color: Theme.surface2
            opacity: root.hovered && !root.isActive ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.fast
                    easing.type: Theme.easing
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: Theme.space1 / 2
            radius: Theme.radiusMd
            color: "transparent"
            border.width: Theme.focusRingWidth
            border.color: Theme.focusRing
            visible: root.visualFocus
        }
    }

    contentItem: Column {
        spacing: Theme.space1
        topPadding: (root.height - Theme.railIconSize - caption.height - Theme.space1) / 2

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: root.iconName
            size: Theme.railIconSize
            color: root.isActive ? Theme.accent : (root.hovered ? Theme.text1 : Theme.text2)
        }
        Text {
            id: caption
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.text
            font.family: Theme.uiFamily
            font.pixelSize: Theme.railCaptionSize
            color: root.isActive ? Theme.text1 : Theme.text3
        }
    }
}
