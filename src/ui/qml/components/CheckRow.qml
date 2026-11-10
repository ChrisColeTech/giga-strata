import QtQuick
import QtQuick.Controls.Basic as Basic

// A tick row of a multi-choice popup (family choice popover, multi mode): a box that shows a
// check while ticked, then the value. Controlled: `isTicked` comes from the owner and a press
// emits `tickToggled(newValue)`; the popup stays open.
Basic.AbstractButton {
    id: root

    property bool isTicked: false

    signal tickToggled(bool isTicked)

    implicitHeight: Theme.menuItemHeight
    implicitWidth: row.implicitWidth + 2 * Theme.space3
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checked: isTicked

    onClicked: root.tickToggled(!root.isTicked)
    Accessible.onToggleAction: root.tickToggled(!root.isTicked)
    Accessible.onPressAction: root.tickToggled(!root.isTicked)

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.hovered || root.visualFocus ? Theme.surface2 : "transparent"
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }
    }

    contentItem: Item {
        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.space3
            spacing: Theme.space2

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSize
                height: Theme.iconSize
                radius: Theme.radiusSm
                color: root.isTicked ? Theme.accentSoft : Theme.field
                border.width: 1
                border.color: root.isTicked ? Theme.accent : Theme.border2

                Icon {
                    anchors.centerIn: parent
                    name: "check"
                    size: Theme.iconSize - Theme.space1
                    color: Theme.accent
                    visible: root.isTicked
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.text
                font.family: Theme.uiFamily
                font.pointSize: Theme.bodySize
                color: Theme.text1
            }
        }
    }
}
