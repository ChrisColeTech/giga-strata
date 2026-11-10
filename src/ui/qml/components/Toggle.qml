import QtQuick
import QtQuick.Controls.Basic as Basic

// S1 switch for a property-grid row (the row's label names it): 36x20 track, off = surface3 + border,
// on = accent, knob slides `switchSlide`. Controlled: `isOn` comes from the owner and a click
// emits `switched(newValue)`.
Basic.AbstractButton {
    id: root

    property bool isOn: false
    property string accessibleName

    signal switched(bool isOn)

    implicitWidth: Theme.switchWidth
    implicitHeight: Theme.propertyControlHeight
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : Theme.disabledOpacity
    Accessible.role: Accessible.CheckBox
    Accessible.name: accessibleName
    Accessible.checked: isOn

    onClicked: root.switched(!root.isOn)

    background: Item {}

    contentItem: Item {
        Rectangle {
            id: track
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.switchWidth
            height: Theme.switchHeight
            radius: height / 2
            color: root.isOn ? Theme.accent : Theme.surface3
            border.width: 1
            border.color: root.isOn ? Theme.accent : (root.hovered && root.enabled ? Theme.border2 : Theme.border)
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

            FocusRing {
                controlRadius: track.radius
                visible: root.visualFocus
            }
        }
    }
}
