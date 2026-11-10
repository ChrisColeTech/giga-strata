pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import GigaStrata.App

// The log drawer at the foot of the Serving screen (04 section 5, row 5): its header holds the
// filter (All / Server / Planner / Warnings), Copy and the chevron that closes it; Clear is in
// the right-click menu. Lines follow C3 (server text2, planner accent, warnings warn, errors
// danger, times text3) and stay pinned to the newest unless scrolled up. Its top edge resizes it.
Rectangle {
    id: root

    readonly property var filters: ["all", "server", "planner", "warnings"]

    signal closeRequested
    signal resized(real delta)

    function lineColor(kind: string): color {
        return kind === "planner" ? Theme.accent
             : kind === "warning" ? Theme.warn
             : kind === "error" ? Theme.danger
             : Theme.text2
    }

    color: Theme.surface1

    EdgeSizer {
        anchors.top: parent.top
        width: parent.width
        z: 1
        onDragged: delta => root.resized(-delta)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.childBorder
        spacing: 0

        RowLayout {
            spacing: Theme.space2
            Layout.fillWidth: true
            Layout.leftMargin: Theme.space3
            Layout.rightMargin: Theme.space1
            Layout.preferredHeight: Theme.controlHeight + Theme.space2

            Overline {
                text: qsTr("Log")
            }

            SegmentedControl {
                model: [qsTr("All"), qsTr("Server"), qsTr("Planner"), qsTr("Warnings")]
                currentIndex: root.filters.indexOf(LogModel.filter)
                onActivated: index => LogModel.filter = root.filters[index]
            }

            Item {
                Layout.fillWidth: true
            }

            GhostButton {
                text: qsTr("Copy")
                enabled: LogModel.lineCount > 0
                toolTipText: LogModel.lineCount > 0 ? qsTr("Copy the shown lines") : qsTr("The log is empty.")
                onClicked: LogModel.copyAll()
            }

            IconButton {
                iconName: "chevron-down"
                toolTipText: qsTr("Close the log drawer")
                onClicked: root.closeRequested()
            }
        }

        Rectangle {
            color: Theme.border
            implicitHeight: 1
            Layout.fillWidth: true
        }

        ListView {
            id: lines

            property bool isPinned: true

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: LogModel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.space3

            onCountChanged: {
                if (lines.isPinned) {
                    lines.positionViewAtEnd()
                }
            }
            onMovementEnded: lines.isPinned = lines.atYEnd

            delegate: RowLayout {
                required property string text
                required property string kind
                required property string time

                width: lines.width
                height: Theme.logLineHeight
                spacing: Theme.space2

                Text {
                    text: parent.time
                    font.family: Theme.monoFamily
                    font.pointSize: Theme.captionSize
                    color: Theme.text3
                }

                Text {
                    text: parent.text
                    font.family: Theme.monoFamily
                    font.pointSize: Theme.captionSize
                    color: root.lineColor(parent.kind)
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Basic.ScrollBar.vertical: Basic.ScrollBar {
                width: Theme.scrollBarWidth
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: logMenu.popup()
            }
        }
    }

    ContextMenu {
        id: logMenu

        ContextMenuItem {
            text: qsTr("Clear")
            enabled: LogModel.lineCount > 0
            onTriggered: LogModel.clear()
        }
    }
}
