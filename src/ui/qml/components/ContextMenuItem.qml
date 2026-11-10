import QtQuick
import QtQuick.Controls.Basic as Basic

// S5 item: h28, Body text1 (danger when destructive), optional 16px icon in text2, shortcut
// hint right-aligned in mono Caption text3; hover or keyboard-current = surface2 radiusSm.
Basic.MenuItem {
    id: root

    property string iconName
    property string shortcutText
    property bool isDestructive: false

    implicitHeight: Theme.menuItemHeight
    leftPadding: Theme.space3
    rightPadding: Theme.space3

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.highlighted ? Theme.surface2 : "transparent"
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }
    }

    contentItem: Row {
        spacing: Theme.space2

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: root.iconName
            color: root.isDestructive ? Theme.danger : Theme.text2
            visible: root.iconName !== ""
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            color: root.isDestructive ? Theme.danger : Theme.text1
        }
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: Theme.space3
        anchors.verticalCenter: parent.verticalCenter
        text: root.shortcutText
        visible: root.shortcutText !== ""
        font.family: Theme.monoFamily
        font.pointSize: Theme.captionSize
        color: Theme.text3
    }
}
