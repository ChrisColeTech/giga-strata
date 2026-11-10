import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// The scrolling stage of a form (the family kit's form/Workspace.tsx): full width, 24px of side
// padding and 32px at the foot, on the workspace color; groups stack in it and bring their own
// top padding and splitters. No centred column, no cards, no boxes.
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
