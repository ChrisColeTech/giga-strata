import QtQuick
import QtQuick.Controls.Basic as Basic

// 6.5 Icon: 30x30, icon only; the tooltip is required because there is no text (A2). `isOn`
// is the family pressed state (accent tint, accent edge, accent glyph) for a lit toggle.
Basic.AbstractButton {
    id: root

    required property string iconName
    required property string toolTipText
    property bool isOn: false
    property int iconSize: Theme.iconSize

    readonly property bool isHot: hovered && enabled

    implicitWidth: Theme.controlHeight
    implicitHeight: Theme.controlHeight
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : Theme.disabledOpacity
    Accessible.name: toolTipText

    background: Rectangle {
        radius: Theme.radiusMd
        color: root.isOn ? Theme.accentTint : (root.isHot ? Theme.surface2 : "transparent")
        border.width: 1
        border.color: root.isOn ? Theme.accentEdge : "transparent"
        Behavior on color {
            ColorAnimation {
                duration: Theme.fast
                easing.type: Theme.easing
            }
        }

        FocusRing {
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: root.iconSize
            color: root.isOn ? Theme.accent : (root.isHot ? Theme.text1 : Theme.text2)
        }
    }

    ThemedToolTip {
        visible: root.hovered
        text: root.toolTipText
    }
}
