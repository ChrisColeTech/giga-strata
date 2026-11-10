import QtQuick
import QtQuick.Controls.Basic as Basic

// S3 toggle chip: h26 pill, tier dot, label plus mono value. Controlled: `isOn` comes from the
// owner and a click emits `switched()`.
Basic.AbstractButton {
    id: root

    property bool isOn: false
    property color dotColor: Theme.text3
    property string valueText
    property string toolTipText

    signal switched

    implicitHeight: Theme.chipHeight
    implicitWidth: row.implicitWidth + 2 * Theme.space3
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.CheckBox
    Accessible.name: text + " " + valueText
    Accessible.checked: isOn

    onClicked: root.switched()

    background: Rectangle {
        radius: height / 2
        color: root.isOn ? Theme.accentSoft : (root.hovered ? Theme.surface3 : Theme.surface2)
        border.width: 1
        border.color: root.isOn ? Theme.accent : (root.hovered ? Theme.border2 : Theme.border)
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }

        FocusRing {
            controlRadius: parent.radius
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row
            anchors.centerIn: parent
            spacing: Theme.space2

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.dotSize
                height: Theme.dotSize
                radius: width / 2
                color: root.dotColor
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.text
                font.family: Theme.uiFamily
                font.pointSize: Theme.captionSize
                color: root.isOn ? Theme.accent : Theme.text2
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.valueText
                visible: root.valueText !== ""
                font.family: Theme.monoFamily
                font.pointSize: Theme.captionSize
                color: root.isOn ? Theme.accent : Theme.text2
            }
        }
    }

    ThemedToolTip {
        visible: root.hovered && root.toolTipText !== ""
        text: root.toolTipText
    }
}
