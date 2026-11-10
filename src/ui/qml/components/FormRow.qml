import QtQuick
import QtQuick.Layouts

// One setting (the family kit's form/Row.tsx): the label in the window's one label column with
// its InfoTip beside it (never text under a control), the control spanning the row, optional
// `trailing` items on the readout edge, and under the control either the error line (danger) or
// a muted mono note. `accent` draws the state bar 12px left of the label column (success,
// error, warning, inactive; never a dot). A row that cannot act yet stays in place, disabled,
// with its reason as the tooltip (`disabledReason`), never a placeholder value.
ColumnLayout {
    id: root

    property string label
    property real labelWidth: Theme.labelColumnMin
    property string toolTipText
    property string errorText
    property string note
    property string accent: "none"
    property string disabledReason
    default property alias control: controls.data
    property alias trailing: trailingRow.data

    readonly property bool isDisabled: disabledReason !== ""

    spacing: Theme.rowGap

    RowLayout {
        id: mainRow
        spacing: Theme.space3
        Layout.fillWidth: true

        Row {
            spacing: Theme.rowGap
            opacity: root.isDisabled ? Theme.disabledOpacity : 1
            Layout.preferredWidth: root.labelWidth
            Layout.minimumWidth: root.labelWidth
            Layout.alignment: Qt.AlignVCenter

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, root.labelWidth - (tip.visible ? Theme.infoTipSize + Theme.rowGap : 0))
                text: root.label
                font.family: Theme.uiFamily
                font.pixelSize: Theme.labelPixelSize
                font.weight: Font.DemiBold
                color: Theme.text1
                elide: Text.ElideRight
            }

            InfoTip {
                id: tip
                anchors.verticalCenter: parent.verticalCenter
                tipText: root.toolTipText
                label: root.label
                visible: root.toolTipText !== ""
            }
        }

        Item {
            implicitHeight: Math.max(Theme.fieldHeight, controls.implicitHeight)
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight

            // The state bar sits in the gutter, 12px left of the label column, so no column moves.
            Rectangle {
                visible: root.accent !== "none"
                x: -root.labelWidth - 2 * Theme.space3
                y: Theme.space1
                width: Theme.rowAccentWidth
                height: parent.height - 2 * Theme.space1
                radius: width / 2
                color: root.accent === "success" ? Theme.success
                     : root.accent === "error" ? Theme.danger
                     : root.accent === "warning" ? Theme.warn
                     : Theme.rowInactive
            }

            RowLayout {
                id: controls
                anchors.fill: parent
                spacing: Theme.space2
                enabled: !root.isDisabled
            }

            HoverHandler {
                id: disabledHover
                enabled: root.isDisabled
            }

            ThemedToolTip {
                visible: disabledHover.hovered && root.isDisabled
                text: root.disabledReason
            }
        }

        RowLayout {
            id: trailingRow
            spacing: Theme.space1
            visible: children.length > 0
        }
    }

    Text {
        readonly property bool isError: root.errorText !== ""
        visible: isError || root.note !== ""
        text: isError ? root.errorText : root.note
        leftPadding: root.labelWidth + Theme.space3
        font.family: isError ? Theme.uiFamily : Theme.monoFamily
        font.pixelSize: isError ? Theme.labelPixelSize : Theme.notePixelSize
        color: isError ? Theme.danger : Theme.text2
        elide: Text.ElideRight
        Layout.fillWidth: true
        Accessible.role: isError ? Accessible.AlertMessage : Accessible.StaticText
    }
}
