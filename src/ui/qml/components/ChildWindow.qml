pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Effects
import QtQuick.Layouts

// The family's authored child window (the kit's windows/ChildWindow.tsx with
// hooks/useChildWindowBox.ts and hooks/useChildWindowKeys.ts), drawn inside the main window: its
// own 1px border on the panel color with the menu shadow and an 8px inset, the 28px header (title,
// Maximize/Restore, Close), eight grips (none while maximized), the body. It opens centred at the
// size its spec wants but never more than 80% of the room, can be made as large as the room and
// no smaller than its least, stays inside, and remembers its normal box and maximized flag for the
// run. A modal one puts a scrim over the main window. Escape follows `escapePolicy` (never, always,
// unless-typing); a field or an open menu has the key first. Closing never commits anything.
Basic.Popup {
    id: root

    property string title
    // The kit's ChildSpec; the defaults are its workSpec (Settings, Model config).
    property int specWidth: Theme.workWidth
    property int specBody: Theme.workBody
    property int specMinWidth: Theme.workLeastWidth
    property int specMinBody: Theme.workLeastBody
    property bool isModal: true
    property string escapePolicy: "unless-typing"
    property bool isMaximizable: true
    default property alias content: body.data

    property Item opener: null
    property var normalBox: null
    property bool isMaximized: false
    property var gestureStart: null

    readonly property var spec: ({ width: specWidth, body: specBody, minWidth: specMinWidth, minBody: specMinBody })
    readonly property var viewport: ({ width: parent ? parent.width : 0, height: parent ? parent.height : 0 })
    readonly property var room: bounds.roomOf(viewport)
    readonly property var limits: bounds.limitsOf(spec, room)
    readonly property var shownBox: isMaximized ? bounds.maximizedBox(viewport)
                                                : (normalBox ?? { x: 0, y: 0, width: 0, height: 0 })

    function show(openerItem: Item) {
        root.opener = openerItem
        root.open()
    }

    function toggleMaximized() {
        if (root.isMaximizable) {
            root.isMaximized = !root.isMaximized
        }
    }

    function beginGesture() {
        root.gestureStart = root.isMaximized ? null : root.normalBox
    }

    function moveBy(deltaX: real, deltaY: real) {
        const start = root.gestureStart
        if (start === null) {
            return
        }
        const moved = bounds.inside(start.x + deltaX, start.y + deltaY, start, root.room)
        root.normalBox = { x: moved.x, y: moved.y, width: start.width, height: start.height }
    }

    function resizeBy(edges: int, deltaX: real, deltaY: real) {
        if (root.gestureStart !== null) {
            root.normalBox = bounds.resized(root.gestureStart, edges, deltaX, deltaY, root.limits, root.room)
        }
    }

    function refit() {
        if (root.normalBox === null) {
            return
        }
        const size = bounds.clampSize(root.normalBox, root.limits)
        const at = bounds.inside(root.normalBox.x, root.normalBox.y, size, root.room)
        root.normalBox = { x: at.x, y: at.y, width: size.width, height: size.height }
    }

    function isTyping(item: Item): bool {
        const input = item as TextInput
        const editor = item as TextEdit
        return (input !== null && !input.readOnly) || (editor !== null && !editor.readOnly)
    }

    parent: Basic.Overlay.overlay
    x: shownBox.x
    y: shownBox.y
    width: shownBox.width
    height: shownBox.height
    margins: -1
    padding: 0
    modal: isModal
    dim: isModal
    focus: true
    closePolicy: Basic.Popup.NoAutoClose

    onAboutToShow: root.normalBox = bounds.openBox(root.spec, root.room, root.normalBox)
    onOpened: frame.forceActiveFocus()
    onClosed: {
        root.gestureStart = null
        if (root.opener && root.opener.visible) {
            root.opener.forceActiveFocus()
        }
    }
    onLimitsChanged: root.refit()

    ChildBounds {
        id: bounds
    }

    Basic.Overlay.modal: Rectangle {
        color: Theme.scrim

        // A press on the scrim brings the window forward and gives it the keyboard.
        MouseArea {
            anchors.fill: parent
            onPressed: frame.forceActiveFocus()
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.childEnter
            easing.type: Theme.easing
        }
        NumberAnimation {
            target: rise
            property: "y"
            from: Theme.motionScale > 0 ? Theme.childRise : 0
            to: 0
            duration: Theme.childEnter
            easing.type: Theme.easing
        }
    }
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Theme.childEnter
            easing.type: Theme.easing
        }
        NumberAnimation {
            target: rise
            property: "y"
            from: 0
            to: Theme.motionScale > 0 ? Theme.childSink : 0
            duration: Theme.childEnter
            easing.type: Theme.easing
        }
    }

    background: null

    contentItem: FocusScope {
        id: frame

        transform: Translate {
            id: rise
        }
        Accessible.role: Accessible.Dialog
        Accessible.name: qsTr("%1 window").arg(root.title)
        Keys.onEscapePressed: event => {
            const isHeldByField = root.escapePolicy === "unless-typing" && root.isTyping(frame.Window.activeFocusItem)
            event.accepted = root.escapePolicy !== "never" && !isHeldByField
            if (event.accepted) {
                root.close()
            }
        }

        Rectangle {
            id: panel
            anchors.fill: parent
            color: Theme.surface1
            border.width: Theme.childBorder
            border.color: Theme.border
            visible: false
        }

        MultiEffect {
            anchors.fill: panel
            source: panel
            shadowEnabled: true
            shadowColor: Theme.shadowMenu
            shadowBlur: Theme.shadowMenuBlur
            shadowVerticalOffset: Theme.shadowMenuOffset
            autoPaddingEnabled: true
        }

        Repeater {
            model: root.isMaximized ? [] : [Qt.TopEdge, Qt.BottomEdge, Qt.LeftEdge, Qt.RightEdge,
                Qt.TopEdge | Qt.LeftEdge, Qt.TopEdge | Qt.RightEdge, Qt.BottomEdge | Qt.LeftEdge,
                Qt.BottomEdge | Qt.RightEdge]
            delegate: ResizeGrip {
                required property int modelData
                edges: modelData
                onStarted: root.beginGesture()
                onDragged: (gripEdges, deltaX, deltaY) => root.resizeBy(gripEdges, deltaX, deltaY)
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.childBorder + Theme.childInset
            spacing: Theme.childHeaderGap

            WindowHeader {
                title: root.title
                isMaximizable: root.isMaximizable
                isMaximized: root.isMaximized
                Layout.fillWidth: true
                onMoveStarted: root.beginGesture()
                onMoved: (deltaX, deltaY) => root.moveBy(deltaX, deltaY)
                onMaximizeToggled: root.toggleMaximized()
                onCloseRequested: root.close()
            }

            Item {
                id: body
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
