pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GigaStrata.App

// Settings > Models (04 sections 3 and 7.3), as a property grid: the model folders the library
// scans, one row each with its remove glyph, and Add a model folder (a native folder picker).
// Every change rescans.
Workspace {
    id: root

    PropertySection {
        title: qsTr("Folders")
        Layout.topMargin: Theme.space1

        Repeater {
            model: ModelLibrary.folders
            delegate: PropertyRow {
                id: folderRow
                required property string modelData
                required property int index

                label: folderRow.index === 0 ? qsTr("Model folders") : ""
                labelWidth: Theme.propertyLabelWideWidth
                isControlWide: true

                Text {
                    text: folderRow.modelData
                    font.family: Theme.monoFamily
                    font.pointSize: Theme.bodySize
                    color: Theme.text1
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                IconButton {
                    iconName: "folder-minus"
                    implicitWidth: Theme.propertyRowHeight
                    implicitHeight: Theme.propertyRowHeight
                    toolTipText: qsTr("Remove %1 from the library").arg(folderRow.modelData)
                    onClicked: ModelLibrary.removeFolder(folderRow.modelData)
                }
            }
        }

        PropertyRow {
            label: ModelLibrary.folders.length === 0 ? qsTr("Model folders") : ""
            labelWidth: Theme.propertyLabelWideWidth

            GhostButton {
                text: qsTr("Add a model folder")
                iconName: "folder-plus"
                implicitHeight: Theme.propertyRowHeight
                // The ghost's inset lines its glyph up with the folder paths above.
                Layout.leftMargin: -Theme.buttonPadding
                onClicked: picker.open()
            }
        }
    }

    FolderDialog {
        id: picker
        title: qsTr("Add a model folder")
        onAccepted: ModelLibrary.addFolder(selectedFolder)
    }
}
