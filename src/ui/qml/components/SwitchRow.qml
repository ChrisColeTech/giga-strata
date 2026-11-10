import QtQuick
import QtQuick.Controls.Basic as Basic

// S1: label left, 36x20 switch right; the whole row is the hit target. Controlled: `isOn`
// comes from the owner and a click emits `switched(newValue)`, so external changes (a loaded
// preset) always show. A disabled row keeps its tooltip to say why (B2).
Basic.AbstractButton {
    id: root

    property bool isOn: false
    property string toolTipText

    signal switched(bool isOn)

    implicitHeight: Theme.controlHeight
    implicitWidth: label.implicitWidth + Theme.space3 + Theme.switchWidth
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checked: isOn

    onClicked: root.switched(!root.isOn)

    background: Item {
        FocusRing {
            controlRadius: Theme.radiusSm
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        Text {
            id: label
            anchors.left: parent.left
            anchors.right: track.left
            anchors.rightMargin: Theme.space3
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            elide: Text.ElideRight
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            font.weight: Font.DemiBold
            color: Theme.text2
            opacity: root.enabled ? 1 : Theme.disabledOpacity
        }

        Rectangle {
            id: track
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.switchWidth
            height: Theme.switchHeight
            radius: height / 2
            color: root.isOn ? Theme.accent : Theme.surface3
            border.width: 1
            border.color: root.isOn ? Theme.accent : (root.hovered && root.enabled ? Theme.border2 : Theme.border)
            opacity: root.enabled ? 1 : Theme.disabledOpacity
            Behavior on color {
                ColorAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }

            Rectangle {
                width: Theme.switchKnobSize
                height: Theme.switchKnobSize
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                x: root.isOn ? parent.width - width - (parent.height - height) / 2 : (parent.height - height) / 2
                color: Theme.knob
                Behavior on x {
                    NumberAnimation {
                        duration: Theme.switchSlide
                        easing.type: Theme.easing
                    }
                }
            }
        }
    }

    // A disabled control receives no hover, so this area keeps the explanation reachable.
    MouseArea {
        id: hoverProbe
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    ThemedToolTip {
        visible: hoverProbe.containsMouse && root.toolTipText !== ""
        text: root.toolTipText
    }
}
