pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GigaStrata.App

// Settings > Models (04 section 3): the model folders the library scans, one row each with its
// remove glyph, and Add a model folder (a native folder picker). Every change rescans.
Workspace {
    id: root

    property real labelWidth
    readonly property string foldersLabel: qsTr("Model folders")
    readonly property var labels: [foldersLabel]

    FormGroup {
        title: qsTr("Folders")
        iconName: "folder-open"
        isLast: true
        Layout.fillWidth: true

        Repeater {
            model: ModelLibrary.folders
            delegate: FormRow {
                id: folderRow
                required property string modelData
                required property int index

                label: folderRow.index === 0 ? root.foldersLabel : ""
                labelWidth: root.labelWidth
                Layout.fillWidth: true

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
                    toolTipText: qsTr("Remove %1 from the library").arg(folderRow.modelData)
                    onClicked: ModelLibrary.removeFolder(folderRow.modelData)
                }
            }
        }

        FormRow {
            label: ModelLibrary.folders.length === 0 ? root.foldersLabel : ""
            labelWidth: root.labelWidth
            Layout.fillWidth: true

            GhostButton {
                text: qsTr("Add a model folder")
                iconName: "folder-plus"
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
