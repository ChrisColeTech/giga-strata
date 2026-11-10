pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import GigaStrata.App

// A Models table column's filter (04 section 2.1, spreadsheet style), opened under its heading:
// Name filters by text as you type; Type, Quant and Files tick the values to show (the popup
// stays open). Clear ends the popup. `ticked` and `text` are the table's per-column memory, so
// a popup reopens with what it showed.
Basic.Popup {
    id: root

    property string role
    property string kind: "ticks"
    property var ticked: []
    property string text
    property var values: []

    signal tickedEdited(var ticked)
    signal textEdited(string text)

    function openUnder(anchorItem: Item, columnRole: string, columnKind: string, columnTicked: var, columnText: string) {
        root.role = columnRole
        root.kind = columnKind
        root.ticked = columnTicked
        root.text = columnText
        root.values = columnKind === "ticks" ? ModelLibrary.filterValues(columnRole) : []
        const at = anchorItem.mapToItem(root.parent, 0, anchorItem.height)
        root.x = at.x
        root.y = at.y
        root.open()
    }

    function shownValue(value: string): string {
        return root.role === "files" && value !== "1" ? qsTr("%1 shards").arg(value) : value
    }

    function toggle(value: string, isTicked: bool) {
        const next = root.ticked.filter(each => each !== value)
        if (isTicked) {
            next.push(value)
        }
        root.ticked = next
        ModelLibrary.setFilter(root.role, next)
        root.tickedEdited(next)
    }

    function clear() {
        ModelLibrary.setFilter(root.role, root.kind === "text" ? "" : [])
        root.ticked = []
        root.tickedEdited([])
        root.textEdited("")
        root.close()
    }

    parent: Basic.Overlay.overlay
    width: Theme.filterPopupWidth
    padding: Theme.space1
    focus: true
    closePolicy: Basic.Popup.CloseOnEscape | Basic.Popup.CloseOnPressOutside

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface3
        border.width: 1
        border.color: Theme.border2
    }

    contentItem: ColumnLayout {
        spacing: Theme.space1

        TextField {
            id: nameField
            placeholderText: qsTr("Part of a name")
            text: root.text
            visible: root.kind === "text"
            Layout.fillWidth: true
            Layout.margins: Theme.space1
        }

        Connections {
            target: nameField.input
            function onTextEdited() {
                ModelLibrary.setFilter(root.role, nameField.input.text)
                root.textEdited(nameField.input.text)
            }
        }

        Repeater {
            model: root.kind === "ticks" ? root.values : []
            delegate: CheckRow {
                required property string modelData
                text: root.shownValue(modelData)
                isTicked: root.ticked.indexOf(modelData) >= 0
                Layout.fillWidth: true
                onTickToggled: isTicked => root.toggle(modelData, isTicked)
            }
        }

        GhostButton {
            text: qsTr("Clear")
            Layout.alignment: Qt.AlignRight
            onClicked: root.clear()
        }
    }
}
