import QtQuick
import QtQuick.Controls.Basic as Basic

// The info glyph beside a form label (the family kit's form/InfoTip.tsx): a 20px focusable glyph,
// dim at rest and accent text on hover or focus, whose tooltip explains the field.
Basic.AbstractButton {
    id: root

    property string tipText
    property string label

    implicitWidth: Theme.infoTipSize
    implicitHeight: Theme.infoTipSize
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    Accessible.name: qsTr("About %1").arg(label)
    Accessible.description: tipText

    background: Item {}

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: "info"
            color: root.hovered || root.visualFocus ? Theme.accentText : Theme.text3
        }
    }

    ThemedToolTip {
        visible: root.hovered || root.visualFocus
        text: root.tipText
    }
}
