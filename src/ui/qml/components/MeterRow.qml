import QtQuick
import QtQuick.Layouts

// One memory meter on a property-grid row (04 sections 7.2 and 7.3): the tier's name, a thin bar
// in its tier color, the mono numbers right-aligned. Used by the Serving readout and the
// Parameters panel's memory bars.
RowLayout {
    id: root

    property string label
    property real fraction: 0
    property color fillColor: Theme.accent
    property string valueText
    property string toolTipText

    spacing: Theme.space2
    Layout.fillWidth: true
    Layout.preferredHeight: Theme.propertyRowHeight

    Text {
        text: root.label
        font.family: Theme.uiFamily
        font.pointSize: Theme.bodySize
        color: Theme.text2
        elide: Text.ElideRight
        Layout.preferredWidth: Theme.readoutLabelWidth

        HoverHandler {
            id: labelHover
        }

        ThemedToolTip {
            visible: labelHover.hovered && root.toolTipText !== ""
            text: root.toolTipText
        }
    }

    UsageBar {
        fraction: root.fraction
        fillColor: root.fillColor
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        text: root.valueText
        font.family: Theme.monoFamily
        font.pointSize: Theme.bodySize
        color: Theme.text1
        horizontalAlignment: Text.AlignRight
        Layout.preferredWidth: Math.max(Theme.gaugeLabelWidth, implicitWidth)
    }
}
