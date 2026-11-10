import QtQuick
import QtQuick.Layouts

// K1 section: Overline + content column. K3: items given to `advanced` sit behind a Ghost
// "Advanced" toggle, collapsed by default, indented space3, height animated on expand.
ColumnLayout {
    id: root

    property string title
    property bool isAdvancedOpen: false
    default property alias content: body.data
    property alias advanced: advancedBody.data

    spacing: Theme.space2

    Overline {
        text: root.title
        Layout.fillWidth: true
    }

    ColumnLayout {
        id: body
        spacing: Theme.space2
        Layout.fillWidth: true
    }

    GhostButton {
        text: qsTr("Advanced")
        iconName: "chevron-down"
        iconRotation: root.isAdvancedOpen ? 0 : -90
        visible: advancedBody.children.length > 0
        Accessible.description: root.isAdvancedOpen ? qsTr("Expanded") : qsTr("Collapsed")
        onClicked: root.isAdvancedOpen = !root.isAdvancedOpen
    }

    Item {
        visible: advancedBody.children.length > 0
        clip: true
        Layout.fillWidth: true
        Layout.preferredHeight: root.isAdvancedOpen ? advancedBody.implicitHeight : 0
        Behavior on Layout.preferredHeight {
            NumberAnimation {
                duration: Theme.normal
                easing.type: Theme.easing
            }
        }

        ColumnLayout {
            id: advancedBody
            x: Theme.space3
            width: parent.width - Theme.space3
            spacing: Theme.space2
            enabled: root.isAdvancedOpen
        }
    }
}
