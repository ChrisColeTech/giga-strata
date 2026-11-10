import QtQuick
import QtQuick.Controls.Basic as Basic

// The window's menu bar (04 section 7.1): File (Model folders, Settings, Exit) and Help (the
// repository's README, the repository, a new issue, About). Themed like the S5 menus; Alt and
// the underlined mnemonics reach it from the keyboard. The window acts on the requests.
Basic.MenuBar {
    id: root

    signal modelFoldersRequested
    signal settingsRequested
    signal exitRequested
    signal aboutRequested

    delegate: Basic.MenuBarItem {
        id: barItem

        readonly property bool isLit: highlighted || (menu !== null && menu.visible)

        implicitHeight: Theme.menuBarHeight
        leftPadding: Theme.space3
        rightPadding: Theme.space3
        Accessible.name: text.replace("&", "")
        // An assistive tool's press opens the menu the way a click on the bar does.
        Accessible.onPressAction: barItem.menu.popup(barItem, 0, barItem.height)

        contentItem: Text {
            text: barItem.text.replace("&", "")
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            color: barItem.isLit ? Theme.text1 : Theme.text2
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: barItem.isLit ? Theme.surface2 : "transparent"
        }
    }

    background: Rectangle {
        implicitHeight: Theme.menuBarHeight
        color: Theme.surface1

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }
    }

    ContextMenu {
        title: qsTr("&File")

        ContextMenuItem {
            text: qsTr("Model folders…")
            onTriggered: root.modelFoldersRequested()
        }
        ContextMenuItem {
            text: qsTr("Settings…")
            shortcutText: "Ctrl+,"
            onTriggered: root.settingsRequested()
        }
        MenuDivider {}
        ContextMenuItem {
            text: qsTr("Exit")
            shortcutText: "Ctrl+Q"
            onTriggered: root.exitRequested()
        }
    }

    ContextMenu {
        title: qsTr("&Help")

        ContextMenuItem {
            text: qsTr("Documentation")
            onTriggered: Qt.openUrlExternally(Theme.repositoryUrl + "#readme")
        }
        ContextMenuItem {
            text: qsTr("GitHub repository")
            onTriggered: Qt.openUrlExternally(Theme.repositoryUrl)
        }
        ContextMenuItem {
            text: qsTr("Report an issue")
            onTriggered: Qt.openUrlExternally(Theme.repositoryUrl + "/issues/new")
        }
        MenuDivider {}
        ContextMenuItem {
            text: qsTr("About Giga Strata")
            onTriggered: root.aboutRequested()
        }
    }
}
