pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// A settings window's rail (the family kit's windows/SettingsRail.tsx and lists/nav-classes.ts
// NAV_ROW): its sections down the left edge, 150 wide and 210 on a wide window, on the panel
// color with a hairline on the right. A row is 34px, icon then label; at rest a transparent 2px
// left edge, hover fill; selected the accent left edge, the control fill and accent text and icon.
Rectangle {
    id: root

    // [{label, icon}]
    property var sections: []
    property int currentIndex: 0
    property bool isWide: false

    implicitWidth: isWide ? Theme.settingsRailWideWidth : Theme.settingsRailWidth
    color: Theme.surface1
    Accessible.role: Accessible.PageTabList
    Accessible.name: qsTr("Settings sections")

    Rectangle {
        anchors.right: parent.right
        width: Theme.childBorder
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.topMargin: Theme.settingsRailPadding
        width: parent.width - Theme.childBorder
        spacing: 0

        Repeater {
            model: root.sections
            delegate: Basic.AbstractButton {
                id: link
                required property var modelData
                required property int index

                readonly property bool isSelected: index === root.currentIndex

                text: modelData.label
                implicitHeight: Theme.railRowHeight
                focusPolicy: Qt.StrongFocus
                Layout.fillWidth: true
                Accessible.role: Accessible.PageTab
                Accessible.name: modelData.label
                Accessible.selected: isSelected
                onClicked: root.currentIndex = index

                background: Rectangle {
                    color: link.isSelected ? Theme.surface2 : (link.hovered ? Theme.surface2 : "transparent")
                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.fast
                            easing.type: Theme.easing
                        }
                    }

                    Rectangle {
                        width: Theme.railEdgeWidth
                        height: parent.height
                        color: link.isSelected ? Theme.accent : "transparent"
                    }

                    FocusRing {
                        anchors.margins: Theme.focusRingWidth
                        controlRadius: 0
                        visible: link.visualFocus
                    }
                }

                contentItem: RowLayout {
                    spacing: Theme.space3

                    Icon {
                        name: link.modelData.icon
                        size: Theme.captionIconSize
                        color: link.isSelected ? Theme.accent : (link.hovered ? Theme.text1 : Theme.text2)
                        Layout.leftMargin: Theme.space3
                    }

                    Text {
                        text: link.text
                        font.family: Theme.uiFamily
                        font.pointSize: Theme.bodySize
                        color: link.isSelected ? Theme.accent : (link.hovered ? Theme.text1 : Theme.text2)
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.rightMargin: Theme.space2
                    }
                }
            }
        }
    }
}
