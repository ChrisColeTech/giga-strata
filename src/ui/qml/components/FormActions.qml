import QtQuick
import QtQuick.Layouts

// A form's end (the family kit's form/Actions.tsx): ghost actions on the left (`leading`), the one
// filled accent on the right (the default content), 8px apart, 4px under the last group.
RowLayout {
    id: root

    default property alias content: right.data
    property alias leading: leftRow.data

    spacing: Theme.space2
    Layout.fillWidth: true
    Layout.topMargin: Theme.space1

    RowLayout {
        id: leftRow
        spacing: Theme.space2
    }

    Item {
        Layout.fillWidth: true
    }

    RowLayout {
        id: right
        spacing: Theme.space2
    }
}
