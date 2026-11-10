import QtQuick
import QtQuick.Controls.Basic as Basic

// The family two-step (`ArmedButton` semantics) as a menu row: at rest it names the verb in
// danger text; the first press arms it into the danger question (`armedText`); a second press
// within `armTimeout` confirms. The timeout, Escape or focus leaving it disarms. It is not a
// MenuItem, so arming does not close the menu; the owner closes it after `confirmed`.
Basic.AbstractButton {
    id: root

    property string armedText
    property bool isArmed: false

    signal confirmed

    function disarm() {
        root.isArmed = false
        armTimer.stop()
    }

    implicitHeight: Theme.menuItemHeight
    implicitWidth: label.implicitWidth + 2 * Theme.space3
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.MenuItem
    Accessible.name: isArmed ? armedText : text

    onClicked: {
        if (!root.isArmed) {
            root.isArmed = true
            armTimer.restart()
            return
        }
        root.disarm()
        root.confirmed()
    }
    onActiveFocusChanged: {
        if (!root.activeFocus) {
            root.disarm()
        }
    }
    onVisibleChanged: root.disarm()
    Keys.onEscapePressed: event => {
        event.accepted = root.isArmed
        root.disarm()
    }

    Timer {
        id: armTimer
        interval: Theme.armTimeout
        onTriggered: root.disarm()
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: root.isArmed ? (root.hovered ? Theme.dangerHover : Theme.danger)
                            : (root.hovered || root.visualFocus ? Theme.surface2 : "transparent")
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }
    }

    contentItem: Text {
        id: label
        leftPadding: Theme.space3
        rightPadding: Theme.space3
        verticalAlignment: Text.AlignVCenter
        text: root.isArmed ? root.armedText : root.text
        font.family: Theme.uiFamily
        font.pointSize: Theme.bodySize
        font.weight: root.isArmed ? Font.DemiBold : Font.Normal
        color: root.isArmed ? Theme.textOnDanger : Theme.danger
    }
}
