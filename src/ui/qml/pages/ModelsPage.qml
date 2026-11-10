import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Models screen (04 section 2): no toolbar, the table fills it. With no model folder set,
// the empty table holds one button, Add a model folder, which opens Settings on Models.
Rectangle {
    id: root

    readonly property bool hasFolders: ModelLibrary.folders.length > 0

    signal configRequested(string path, string name, Item opener)
    signal addFolderRequested(Item opener)

    color: Theme.bg

    ModelTable {
        anchors.fill: parent
        anchors.margins: Theme.space2
        onConfigRequested: (path, name, opener) => root.configRequested(path, name, opener)
    }

    SecondaryButton {
        id: addFolder
        anchors.centerIn: parent
        text: qsTr("Add a model folder")
        iconName: "folder-plus"
        visible: !root.hasFolders
        onClicked: root.addFolderRequested(addFolder)
    }

    Connections {
        target: ModelLibrary
        function onErrorOccurred(message: string) {
            toasts.show(message, "error")
        }
    }

    ToastCenter {
        id: toasts
        anchors.fill: parent
    }
}
