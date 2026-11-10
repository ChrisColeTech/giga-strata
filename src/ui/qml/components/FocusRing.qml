import QtQuick

// C5: 2px focusRing drawn 2px outside the control it decorates; keyboard focus only, so the
// owner binds `visible` to its visualFocus.
Rectangle {
    property real controlRadius: Theme.radiusMd

    anchors.fill: parent
    anchors.margins: -2 * Theme.focusRingWidth
    radius: controlRadius + 2 * Theme.focusRingWidth
    color: "transparent"
    border.width: Theme.focusRingWidth
    border.color: Theme.focusRing
}
