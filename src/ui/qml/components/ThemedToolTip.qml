import QtQuick
import QtQuick.Controls.Basic as Basic

// The one tooltip look (6.3 Caption on a raised surface). The Basic default is a light box
// that breaks the palette, so every control instantiates this instead of ToolTip.text.
Basic.ToolTip {
    id: root

    delay: Theme.toolTipDelay
    padding: Theme.space2

    contentItem: Text {
        text: root.text
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.text1
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        color: Theme.surface3
        border.color: Theme.border2
        radius: Theme.radiusSm
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.fast
            easing.type: Theme.easing
        }
    }
}
