import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// F1/F2 field: label above, inset field, inline error below. Controlled: `text` comes from the
// owner (a SettingsStore property); edits leave through `committed` on Enter or focus-out (F5),
// after `validate` (text -> error sentence, empty when valid; derived fields redefine it) has
// accepted them.
ColumnLayout {
    id: root

    property string label
    property string text
    property string placeholderText
    property string toolTipText
    property string errorText
    property bool isMono: false
    property bool isNumeric: false
    property int fieldHeight: Theme.fieldHeight
    property alias input: input
    default property alias trailing: trailingRow.data

    signal committed(string text)

    spacing: Theme.space1
    Layout.alignment: Qt.AlignTop

    onTextChanged: input.text = text
    Component.onCompleted: input.text = text

    FieldLabel {
        text: root.label
        visible: root.label !== ""
    }

    RowLayout {
        id: trailingRow
        spacing: Theme.space2
        Layout.fillWidth: true

        Basic.TextField {
            id: input

            Layout.fillWidth: true
            implicitHeight: root.fieldHeight
            leftPadding: Theme.space3
            rightPadding: Theme.space3
            verticalAlignment: TextInput.AlignVCenter
            horizontalAlignment: root.isNumeric ? TextInput.AlignRight : TextInput.AlignLeft
            placeholderText: root.placeholderText
            placeholderTextColor: Theme.text3
            font.family: root.isMono ? Theme.monoFamily : Theme.uiFamily
            font.pointSize: Theme.bodySize
            color: Theme.text1
            selectionColor: Theme.accentSoft
            selectedTextColor: Theme.text1
            selectByMouse: true
            Accessible.name: root.label

            onEditingFinished: root.commit()
            Keys.onEscapePressed: {
                input.text = root.text
                root.errorText = ""
                input.focus = false
            }

            background: Rectangle {
                radius: Theme.radiusMd
                color: Theme.field
                border.width: 1
                border.color: root.errorText !== "" ? Theme.danger
                    : (input.activeFocus ? Theme.accent : (input.hovered ? Theme.border2 : Theme.border))
                Behavior on border.color {
                    ColorAnimation {
                        duration: Theme.fast
                        easing.type: Theme.easing
                    }
                }

                FocusRing {
                    visible: input.activeFocus && (input.focusReason === Qt.TabFocusReason || input.focusReason === Qt.BacktabFocusReason)
                }
            }

            ThemedToolTip {
                visible: input.hovered && !input.activeFocus && root.toolTipText !== ""
                text: root.toolTipText
            }
        }
    }

    Text {
        text: root.errorText
        visible: root.errorText !== ""
        font.family: Theme.uiFamily
        font.pointSize: Theme.captionSize
        color: Theme.danger
        elide: Text.ElideRight
        Layout.fillWidth: true
    }

    function validate(_value: string): string {
        return ""
    }

    function commit() {
        const error = root.validate(input.text)
        root.errorText = error
        if (error === "" && input.text !== root.text) {
            root.committed(input.text)
            // The owner may normalize the value (trim, "" -> auto); show what was stored.
            input.text = root.text
        }
    }
}
