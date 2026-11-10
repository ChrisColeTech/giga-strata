import QtQuick
import QtQuick.Controls.Basic as Basic

// Shared body of the four text buttons (6.5 table). Only the variants are used in pages; each
// sets its colors here so hover, press, disabled and focus behave identically (B2, C5).
Basic.AbstractButton {
    id: root

    property color fillColor: "transparent"
    property color hoverFillColor: fillColor
    property color pressedFillColor: hoverFillColor
    property color borderColor: "transparent"
    property color hoverBorderColor: borderColor
    property color textColor: Theme.text1
    property color hoverTextColor: textColor
    property int fontWeight: Font.Normal
    property string iconName
    property real iconRotation: 0
    property string toolTipText

    readonly property bool isHot: hovered && enabled

    implicitHeight: Theme.controlHeight
    implicitWidth: contentItem.implicitWidth + 2 * Theme.buttonPadding
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : Theme.disabledOpacity
    Accessible.name: text

    background: Rectangle {
        radius: Theme.radiusMd
        color: root.down ? root.pressedFillColor : (root.isHot ? root.hoverFillColor : root.fillColor)
        border.width: 1
        border.color: root.isHot ? root.hoverBorderColor : root.borderColor
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
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row
            anchors.centerIn: parent
            spacing: Theme.space2

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: root.iconName
                color: label.color
                rotation: root.iconRotation
                Behavior on rotation {
                    NumberAnimation {
                        duration: Theme.normal
                        easing.type: Theme.easing
                    }
                }
                visible: root.iconName !== ""
            }
            Text {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                text: root.text
                font.family: Theme.uiFamily
                font.pointSize: Theme.bodySize
                font.weight: root.fontWeight
                color: root.isHot ? root.hoverTextColor : root.textColor
                Behavior on color {
                    ColorAnimation {
                        duration: Theme.fast
                        easing.type: Theme.easing
                    }
                }
            }
        }
    }

    ThemedToolTip {
        visible: root.hovered && root.toolTipText !== ""
        text: root.toolTipText
    }
}
