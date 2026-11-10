import QtQuick

// P1e bar: a 4px surface3 track with a fill for `fraction` (0..1) in `fillColor` (a tier color
// for memory, accent otherwise, C4). A negative fraction is indeterminate: a sliding segment.
Rectangle {
    id: root

    property real fraction: 0
    property color fillColor: Theme.accent
    readonly property bool isIndeterminate: fraction < 0

    implicitHeight: Theme.barHeight
    radius: height / 2
    color: Theme.surface3
    clip: true

    Rectangle {
        height: parent.height
        radius: parent.radius
        width: root.isIndeterminate ? parent.width / 4 : parent.width * Math.max(0, Math.min(1, root.fraction))
        color: root.fillColor
        Behavior on width {
            enabled: !root.isIndeterminate
            NumberAnimation {
                duration: Theme.normal
                easing.type: Theme.easing
            }
        }

        SequentialAnimation on x {
            running: root.isIndeterminate && root.visible && Theme.motionScale > 0
            loops: Animation.Infinite
            NumberAnimation {
                from: -root.width / 4
                to: root.width
                duration: Theme.indeterminateCycle
            }
        }
    }
}
