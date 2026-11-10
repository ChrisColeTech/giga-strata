import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// A section of the property grid (04 section 7.3): a small-caps label on a ~28px row over its
// PropertyRows, no tile, no title type. A collapsible one (ADVANCED) leads its label with a
// chevron and folds its rows; it starts as `isOpen` says.
ColumnLayout {
    id: root

    property string title
    property bool isCollapsible: false
    property bool isOpen: true
    default property alias rows: body.data

    spacing: 0
    Layout.fillWidth: true
    Accessible.role: Accessible.Grouping
    Accessible.name: title

    Basic.AbstractButton {
        id: heading
        enabled: root.isCollapsible
        focusPolicy: root.isCollapsible ? Qt.StrongFocus : Qt.NoFocus
        implicitHeight: Theme.propertyRowHeight
        Layout.fillWidth: true
        Accessible.role: root.isCollapsible ? Accessible.Button : Accessible.StaticText
        Accessible.name: !root.isCollapsible ? root.title
                       : root.isOpen ? qsTr("Collapse %1").arg(root.title) : qsTr("Expand %1").arg(root.title)
        onClicked: root.isOpen = !root.isOpen

        background: FocusRing {
            visible: heading.visualFocus
        }

        contentItem: Row {
            spacing: Theme.space1

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.isCollapsible
                name: "chevron-down"
                size: Theme.iconSize - Theme.space1
                color: heading.hovered ? Theme.text1 : Theme.text2
                rotation: root.isOpen ? 0 : -90
                Behavior on rotation {
                    NumberAnimation {
                        duration: Theme.groupFold
                        easing.type: Theme.easing
                    }
                }
            }

            Overline {
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: heading.hovered && root.isCollapsible ? Theme.text1 : Theme.text2
            }
        }
    }

    ColumnLayout {
        id: body
        visible: root.isOpen
        spacing: 0
        Layout.fillWidth: true
    }
}
