import QtQuick
import QtQuick.Layouts

// A child window's header (the family kit's windows/ChildWindow.tsx header): 28px, a hairline
// under it, the title on the left, then a ghost Maximize/Restore toggle (lit while maximized)
// and a ghost Close, both with the VS Code caption glyphs. Dragging the rest of it moves the
// window (travel since the press, in scene pixels); a double-click toggles maximized.
Item {
    id: root

    property string title
    property bool isMaximizable: true
    property bool isMaximized: false
    property point pressPoint

    signal moveStarted
    signal moved(real deltaX, real deltaY)
    signal moveFinished
    signal maximizeToggled
    signal closeRequested

    implicitHeight: Theme.childTitleHeight

    MouseArea {
        id: dragArea
        anchors.fill: parent
        onPressed: mouse => {
            root.pressPoint = dragArea.mapToItem(null, mouse.x, mouse.y)
            root.moveStarted()
        }
        onPositionChanged: mouse => {
            const scenePoint = dragArea.mapToItem(null, mouse.x, mouse.y)
            root.moved(scenePoint.x - root.pressPoint.x, scenePoint.y - root.pressPoint.y)
        }
        onReleased: root.moveFinished()
        onDoubleClicked: {
            if (root.isMaximizable) {
                root.maximizeToggled()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: Theme.childBorder
        spacing: Theme.space2

        Text {
            leftPadding: Theme.space1
            text: root.title
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            font.weight: Font.DemiBold
            color: Theme.text1
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        IconButton {
            readonly property string label: (root.isMaximized ? qsTr("Restore %1") : qsTr("Maximize %1")).arg(root.title)

            iconName: root.isMaximized ? "chrome-restore" : "chrome-maximize"
            iconSize: Theme.captionIconSize
            isOn: root.isMaximized
            toolTipText: label
            visible: root.isMaximizable
            implicitWidth: Theme.childTitleHeight
            implicitHeight: Theme.childTitleHeight
            onClicked: root.maximizeToggled()
        }

        IconButton {
            iconName: "chrome-close"
            iconSize: Theme.captionIconSize
            toolTipText: qsTr("Close %1").arg(root.title)
            implicitWidth: Theme.childTitleHeight
            implicitHeight: Theme.childTitleHeight
            onClicked: root.closeRequested()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.childBorder
        color: Theme.border
    }
}
