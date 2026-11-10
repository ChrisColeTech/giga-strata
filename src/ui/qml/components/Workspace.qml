import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// The scrolling stage of a property grid (04 section 7.3, after the family kit's
// form/Workspace.tsx): full width, 16px of side padding and 8px at the foot, on the workspace
// color; PropertySections stack in it. No centred column, no cards, no boxes.
Flickable {
    id: root

    default property alias content: column.data

    contentWidth: width
    contentHeight: column.implicitHeight + Theme.workspaceBottom
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    Rectangle {
        parent: root
        z: -1
        anchors.fill: parent
        color: Theme.workspace
    }

    ColumnLayout {
        id: column
        x: Theme.workspacePadding
        width: root.width - 2 * Theme.workspacePadding
        spacing: 0
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
