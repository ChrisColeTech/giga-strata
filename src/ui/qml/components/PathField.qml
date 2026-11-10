import QtQuick
import QtQuick.Dialogs

// F3 path field: while not editing it shows the file name, middle-elided, with the full path in
// the tooltip; a trailing folder button opens the native picker (D3). The picked path is
// committed like a typed one.
TextField {
    id: root

    property string dialogTitle
    property var nameFilters: [qsTr("GGUF files (*.gguf)"), qsTr("All files (*)")]
    property url startFolder

    signal picked(url fileUrl)

    toolTipText: text
    input.color: input.activeFocus ? Theme.text1 : "transparent"

    Text {
        parent: root.input
        anchors.fill: parent
        anchors.leftMargin: root.input.leftPadding
        anchors.rightMargin: root.input.rightPadding
        visible: !root.input.activeFocus && root.input.text !== ""
        verticalAlignment: Text.AlignVCenter
        text: root.input.text.split(/[\\/]/).pop()
        elide: Text.ElideMiddle
        font: root.input.font
        color: Theme.text1
    }

    IconButton {
        iconName: "folder"
        toolTipText: Theme.textBrowse
        onClicked: picker.open()
    }

    FileDialog {
        id: picker
        title: root.dialogTitle
        nameFilters: root.nameFilters
        currentFolder: root.startFolder
        onAccepted: root.picked(selectedFile)
    }
}
