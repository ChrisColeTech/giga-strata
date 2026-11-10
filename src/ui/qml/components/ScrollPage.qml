import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// A column that scrolls vertically with page padding (6.4) and the themed thin scrollbar
// (surface3 thumb, 6.2). Pages and the monitor column share it instead of each styling one.
Flickable {
    id: root

    property int gap: Theme.space4
    default property alias content: column.data

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * Theme.space4
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ColumnLayout {
        id: column
        x: Theme.space4
        y: Theme.space4
        width: root.width - 2 * Theme.space4
        spacing: root.gap
    }

    Basic.ScrollBar.vertical: Basic.ScrollBar {
        id: bar
        policy: root.contentHeight > root.height ? Basic.ScrollBar.AsNeeded : Basic.ScrollBar.AlwaysOff
        width: Theme.scrollBarWidth
        padding: 1

        contentItem: Rectangle {
            radius: width / 2
            color: bar.pressed ? Theme.border2 : Theme.surface3
            opacity: bar.active ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }
        }
    }
}
