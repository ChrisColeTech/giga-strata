import QtQuick

// One of a child window's eight grips (the family kit's windows/ChildGrips.tsx): an edge or a
// corner `childGrip` wide, named by `edges` (Qt.Edge flags). Edges stop `childGrip` short of the
// corners, which own their squares. A gesture reports the pointer's travel since its press in
// scene pixels, so the window resizes from the box it had when the press began.
MouseArea {
    id: root

    required property int edges

    readonly property bool isLeft: (edges & Qt.LeftEdge) !== 0
    readonly property bool isRight: (edges & Qt.RightEdge) !== 0
    readonly property bool isTop: (edges & Qt.TopEdge) !== 0
    readonly property bool isBottom: (edges & Qt.BottomEdge) !== 0
    readonly property bool isSide: isLeft || isRight
    readonly property bool isEnd: isTop || isBottom
    property point pressPoint

    signal started
    signal dragged(int edges, real deltaX, real deltaY)
    signal finished

    x: isRight ? parent.width - width : (isSide ? 0 : Theme.childGrip)
    y: isBottom ? parent.height - height : (isEnd ? 0 : Theme.childGrip)
    width: isSide ? Theme.childGrip : parent.width - 2 * Theme.childGrip
    height: isEnd ? Theme.childGrip : parent.height - 2 * Theme.childGrip
    cursorShape: isSide && isEnd ? ((isLeft && isTop) || (isRight && isBottom) ? Qt.SizeFDiagCursor : Qt.SizeBDiagCursor)
                                 : (isSide ? Qt.SizeHorCursor : Qt.SizeVerCursor)

    onPressed: mouse => {
        root.pressPoint = root.mapToItem(null, mouse.x, mouse.y)
        root.started()
    }
    onPositionChanged: mouse => {
        const scenePoint = root.mapToItem(null, mouse.x, mouse.y)
        root.dragged(root.edges, scenePoint.x - root.pressPoint.x, scenePoint.y - root.pressPoint.y)
    }
    onReleased: root.finished()
}
