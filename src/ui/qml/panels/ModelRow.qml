pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// One model in the Models table (04 sections 2.1 and 2.2): its cells, then Serve, the gear (its
// Model config window) and More (Copy identifier, Reveal in Explorer, Edit config.json, Delete as
// the two-step), which a right-click anywhere on the row also opens, at the pointer.
// No badges: serving state lives in the status bar only.
Rectangle {
    id: root

    required property string name
    required property string path
    required property string type
    required property string quant
    required property double sizeBytes
    required property int experts
    required property int files
    required property date modified
    required property var columns
    property var widths: []
    property bool isNameFilling: true
    property real nameMinimumWidth: Theme.columnNameMinWidth
    property real nameMaximumWidth: Number.POSITIVE_INFINITY

    readonly property var cellTexts: ({
        name: name,
        type: type,
        quant: quant,
        sizeBytes: qsTr("%1 GB").arg((sizeBytes / (1024 * 1024 * 1024)).toFixed(1)),
        experts: experts > 0 ? String(experts) : "—",
        files: files === 1 ? "1" : qsTr("%1 shards").arg(files),
        modified: Qt.formatDateTime(modified, "yyyy-MM-dd HH:mm")
    })

    signal configRequested(string path, string name, Item opener)

    implicitHeight: Theme.tableRowHeight
    radius: Theme.radiusSm
    color: rowHover.hovered ? Theme.surface2 : "transparent"

    HoverHandler {
        id: rowHover
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => moreMenu.popup(root, eventPoint.position.x, eventPoint.position.y)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space3
        spacing: Theme.space3

        Repeater {
            model: root.columns
            delegate: Text {
                required property var modelData
                required property int index
                readonly property bool isName: modelData.role === "name"
                readonly property real columnWidth: root.widths[index] ?? modelData.width

                text: root.cellTexts[modelData.role]
                font.family: modelData.isNumber || modelData.role === "modified" ? Theme.monoFamily : Theme.uiFamily
                font.pointSize: isName ? Theme.bodySize : Theme.captionSize
                font.weight: isName ? Font.DemiBold : Font.Normal
                color: isName ? Theme.text1 : Theme.text2
                elide: Text.ElideRight
                horizontalAlignment: modelData.isNumber ? Text.AlignRight : Text.AlignLeft
                Layout.fillWidth: isName
                Layout.minimumWidth: isName ? root.nameMinimumWidth : columnWidth
                Layout.preferredWidth: columnWidth
                Layout.maximumWidth: isName ? root.nameMaximumWidth : columnWidth
            }
        }

        Item {
            visible: !root.isNameFilling
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: 0
            Layout.preferredWidth: Theme.columnActionsWidth

            GhostButton {
                text: qsTr("Serve")
                iconName: "play"
                toolTipText: qsTr("Serve %1, stopping any model that is serving").arg(root.name)
                onClicked: LaunchController.serve(root.path)
            }

            IconButton {
                id: gear
                iconName: "settings"
                toolTipText: qsTr("Model config for %1").arg(root.name)
                onClicked: root.configRequested(root.path, root.name, gear)
            }

            IconButton {
                id: more
                iconName: "ellipsis"
                toolTipText: qsTr("More actions for %1").arg(root.name)
                onClicked: moreMenu.popup(more, 0, more.height)
            }
        }
    }

    ContextMenu {
        id: moreMenu

        // The armed question is longer than the verb; the menu grows to hold it.
        width: Math.max(Theme.menuMinWidth, armedDelete.implicitWidth + 2 * padding)

        ContextMenuItem {
            text: qsTr("Copy identifier")
            onTriggered: ModelLibrary.copyIdentifier(root.path)
        }
        ContextMenuItem {
            text: qsTr("Reveal in Explorer")
            onTriggered: ModelLibrary.revealInExplorer(root.path)
        }
        ContextMenuItem {
            text: qsTr("Edit config.json")
            onTriggered: ModelLibrary.openConfigFile(root.path)
        }
        ArmedMenuItem {
            id: armedDelete
            text: qsTr("Delete")
            armedText: root.files === 1 ? qsTr("Delete %1? (1 file)").arg(root.name)
                                        : qsTr("Delete %1? (%2 files)").arg(root.name).arg(root.files)
            width: parent ? parent.width : implicitWidth
            onConfirmed: {
                moreMenu.close()
                ModelLibrary.deleteModel(root.path)
            }
        }
    }
}
