import QtQuick
import QtQuick.Controls.Basic as Basic

// S5 right-click menu: surface3 panel, 1px border2, radiusMd, pad 4, no shadow, fades via fast.
// Holds ContextMenuItems; Esc closes, arrows move, Enter activates (Menu behavior).
Basic.Menu {
    id: root

    padding: Theme.space1
    implicitWidth: Math.max(Theme.menuMinWidth, contentItem.implicitWidth + 2 * padding)

    background: Rectangle {
        implicitWidth: Theme.menuMinWidth
        radius: Theme.radiusMd
        color: Theme.surface3
        border.width: 1
        border.color: Theme.border2
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.fast
            easing.type: Theme.easing
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Theme.fast
            easing.type: Theme.easing
        }
    }
}
