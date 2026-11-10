import QtQuick
import QtQuick.Layouts

// One row of the property grid (04 section 7.3): a ~28px row, the label in the grid's label
// column, the control, then a short status word in text3. Reasons live in tooltips: hovering the
// label explains the setting, and a row that cannot act yet stays in place, disabled, with its
// reason (`disabledReason`) as the tooltip. A control that should span the row sets
// `isControlWide`; otherwise it keeps its own width and the status word follows it.
RowLayout {
    id: root

    property string label
    property real labelWidth: Theme.propertyLabelWidth
    property string toolTipText
    property string status
    property string disabledReason
    property bool isControlWide: false
    default property alias control: controls.data

    readonly property bool isDisabled: disabledReason !== ""
    readonly property string reason: isDisabled ? disabledReason : toolTipText

    spacing: Theme.space2
    Layout.fillWidth: true
    Layout.minimumHeight: Theme.propertyRowHeight

    Text {
        text: root.label
        opacity: root.isDisabled ? Theme.disabledOpacity : 1
        font.family: Theme.uiFamily
        font.pointSize: Theme.bodySize
        color: Theme.text2
        elide: Text.ElideRight
        Layout.preferredWidth: root.labelWidth
        Layout.minimumWidth: root.labelWidth
        Layout.maximumWidth: root.labelWidth

        HoverHandler {
            id: labelHover
        }

        ThemedToolTip {
            visible: labelHover.hovered && root.reason !== ""
            text: root.reason
        }
    }

    RowLayout {
        id: controls
        spacing: Theme.space2
        enabled: !root.isDisabled
        Layout.fillWidth: root.isControlWide

        HoverHandler {
            id: controlHover
            enabled: root.isDisabled
        }

        ThemedToolTip {
            visible: controlHover.hovered && root.isDisabled
            text: root.disabledReason
        }
    }

    Text {
        text: root.status
        visible: text !== ""
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.text3
    }
}
