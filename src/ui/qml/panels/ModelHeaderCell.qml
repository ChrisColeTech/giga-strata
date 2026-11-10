import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import GigaStrata.App

// One heading of the Models table (04 section 2.1): a press sorts by its column (again to
// reverse) and shows the direction arrow; a filterable column ends with its filter glyph, filled
// while that column is filtered. A right-click reports itself so the table offers Clear all.
Basic.AbstractButton {
    id: root

    // {role, title, width, isNumber, filter: "" | "text" | "ticks"}
    required property var column
    property string sortRole
    property bool isAscending: true

    readonly property bool isSorted: sortRole === column.role
    readonly property bool isFiltered: ModelLibrary.activeFilters.indexOf(column.role) >= 0

    signal sortRequested(string role)
    signal filterRequested(Item anchor)
    signal menuRequested(real sceneX, real sceneY)

    implicitHeight: Theme.tableHeaderHeight
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.ColumnHeader
    Accessible.name: column.title

    onClicked: root.sortRequested(root.column.role)

    background: Rectangle {
        color: root.hovered ? Theme.surface2 : "transparent"

        FocusRing {
            anchors.margins: Theme.focusRingWidth
            controlRadius: 0
            visible: root.visualFocus
        }
    }

    contentItem: RowLayout {
        spacing: Theme.space1
        layoutDirection: root.column.isNumber ? Qt.RightToLeft : Qt.LeftToRight

        Text {
            text: root.column.title
            font.family: Theme.monoFamily
            font.pointSize: Theme.captionSize
            color: root.isSorted ? Theme.text1 : Theme.text3
            elide: Text.ElideRight
            Layout.fillWidth: !root.column.isNumber
        }

        Icon {
            name: root.isAscending ? "arrow-up" : "arrow-down"
            size: Theme.captionIconSize - Theme.space1
            color: Theme.text2
            visible: root.isSorted
        }

        Item {
            Layout.fillWidth: root.column.isNumber
        }

        IconButton {
            id: filterButton
            iconName: root.isFiltered ? "filter-filled" : "filter"
            iconSize: Theme.captionIconSize - Theme.space1
            isOn: root.isFiltered
            toolTipText: qsTr("Filter %1").arg(root.column.title)
            visible: root.column.filter !== ""
            implicitWidth: Theme.tableHeaderHeight - Theme.space1
            implicitHeight: Theme.tableHeaderHeight - Theme.space1
            onClicked: root.filterRequested(filterButton)
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => root.menuRequested(eventPoint.scenePosition.x, eventPoint.scenePosition.y)
    }
}
