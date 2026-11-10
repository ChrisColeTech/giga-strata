import QtQuick
import QtQuick.Layouts

// K2 card: one monitor widget, one preset or one dialog block; never a grouping box on a page.
Rectangle {
    id: root

    default property alias content: column.data
    property alias spacing: column.spacing

    implicitHeight: column.implicitHeight + 2 * Theme.space3
    implicitWidth: column.implicitWidth + 2 * Theme.space3
    radius: Theme.radiusMd
    color: Theme.surface2
    border.width: 1
    border.color: Theme.border

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: Theme.space3
        spacing: Theme.space2
    }
}
