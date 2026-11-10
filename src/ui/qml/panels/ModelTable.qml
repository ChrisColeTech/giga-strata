pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import GigaStrata.App

// The Models table (04 section 2.1): a sticky 29px heading row over 36px rows that scroll inside
// the table. Every heading sorts; Name, Type, Quant and Files filter from their heading; a
// right-click on the heading row offers Clear all filters. Rows come from ModelLibrary.tableModel
// (05 section 1), already sorted and filtered there.
ColumnLayout {
    id: root

    readonly property var columns: [
        { role: "name", title: qsTr("Name"), width: Theme.columnNameMinWidth, isNumber: false, filter: "text" },
        { role: "type", title: qsTr("Type"), width: Theme.columnTypeWidth, isNumber: false, filter: "ticks" },
        { role: "quant", title: qsTr("Quant"), width: Theme.columnQuantWidth, isNumber: false, filter: "ticks" },
        { role: "sizeBytes", title: qsTr("Size"), width: Theme.columnSizeWidth, isNumber: true, filter: "" },
        { role: "experts", title: qsTr("Experts"), width: Theme.columnExpertsWidth, isNumber: true, filter: "" },
        { role: "files", title: qsTr("Files"), width: Theme.columnFilesWidth, isNumber: false, filter: "ticks" },
        { role: "modified", title: qsTr("Modified"), width: Theme.columnModifiedWidth, isNumber: false, filter: "" }
    ]
    property string sortRole: "name"
    property bool isAscending: true
    // Per-column filter memory for the popups: {role: [ticked values]} and the name text.
    property var tickedByRole: ({})
    property string nameText

    signal configRequested(string path, string name, Item opener)

    function sort(role: string) {
        root.isAscending = role === root.sortRole ? !root.isAscending : true
        root.sortRole = role
        ModelLibrary.sortBy(role, root.isAscending)
    }

    function clearAll() {
        ModelLibrary.clearFilters()
        root.tickedByRole = {}
        root.nameText = ""
    }

    spacing: 0

    Rectangle {
        color: Theme.surface1
        implicitHeight: Theme.tableHeaderHeight
        Layout.fillWidth: true

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.space3
            spacing: Theme.space3

            Repeater {
                model: root.columns
                delegate: ModelHeaderCell {
                    required property var modelData
                    column: modelData
                    sortRole: root.sortRole
                    isAscending: root.isAscending
                    Layout.fillWidth: modelData.role === "name"
                    Layout.minimumWidth: modelData.width
                    Layout.preferredWidth: modelData.width
                    Layout.fillHeight: true
                    onSortRequested: role => root.sort(role)
                    onFilterRequested: anchor => filterPopup.openUnder(anchor, modelData.role, modelData.filter,
                                                                       root.tickedByRole[modelData.role] ?? [],
                                                                       root.nameText)
                    onMenuRequested: (sceneX, sceneY) => headerMenu.popup()
                }
            }

            Item {
                Layout.preferredWidth: Theme.columnActionsWidth
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }
    }

    ListView {
        id: rows
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: ModelLibrary.tableModel
        Layout.fillWidth: true
        Layout.fillHeight: true

        delegate: ModelRow {
            width: rows.width
            columns: root.columns
            onConfigRequested: (path, name, opener) => root.configRequested(path, name, opener)
        }

        Basic.ScrollBar.vertical: Basic.ScrollBar {
            width: Theme.scrollBarWidth
            policy: rows.contentHeight > rows.height ? Basic.ScrollBar.AsNeeded : Basic.ScrollBar.AlwaysOff
        }
    }

    ContextMenu {
        id: headerMenu

        ContextMenuItem {
            text: qsTr("Clear all filters")
            enabled: ModelLibrary.activeFilters.length > 0
            onTriggered: root.clearAll()
        }
    }

    FilterPopup {
        id: filterPopup
        onTickedEdited: ticked => {
            const next = Object.assign({}, root.tickedByRole)
            next[filterPopup.role] = ticked
            root.tickedByRole = next
        }
        onTextEdited: text => root.nameText = text
    }
}
