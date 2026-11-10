import QtQuick

// The drag edge of a resizable drawer or panel: a 1px border line in a wider hit area, border2
// on hover, accent while dragged. Reports pointer travel along its axis in scene pixels.
MouseArea {
    id: root

    property bool isVertical: false
    property real lastScenePosition

    signal dragged(real delta)

    implicitWidth: isVertical ? Theme.resizeGripSize : 0
    implicitHeight: isVertical ? 0 : Theme.resizeGripSize
    hoverEnabled: true
    cursorShape: isVertical ? Qt.SizeHorCursor : Qt.SizeVerCursor

    onPressed: mouse => {
        const scenePoint = root.mapToItem(null, mouse.x, mouse.y)
        root.lastScenePosition = root.isVertical ? scenePoint.x : scenePoint.y
    }
    onPositionChanged: mouse => {
        if (!root.pressed) {
            return
        }
        const scenePoint = root.mapToItem(null, mouse.x, mouse.y)
        const position = root.isVertical ? scenePoint.x : scenePoint.y
        root.dragged(position - root.lastScenePosition)
        root.lastScenePosition = position
    }

    Rectangle {
        anchors.centerIn: parent
        width: root.isVertical ? 1 : parent.width
        height: root.isVertical ? parent.height : 1
        color: root.pressed ? Theme.accent : (root.containsMouse ? Theme.border2 : Theme.border)
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }
    }
}
