import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// D1-D4 modal dialog: scrim, surface2 panel, Title, body content, Secondary Cancel left of the
// one Primary (or Danger for destructive confirmations, named after the action). Esc cancels,
// Enter accepts; focus starts on `initialFocusItem` when given, else on the action button.
Basic.Popup {
    id: root

    property string title
    property string acceptText
    property bool isDestructive: false
    property Item initialFocusItem: null
    default property alias content: body.data

    signal accepted

    parent: Basic.Overlay.overlay
    anchors.centerIn: parent
    width: Theme.dialogWidth
    padding: Theme.space4
    modal: true
    focus: true
    closePolicy: Basic.Popup.CloseOnEscape

    onOpened: (root.initialFocusItem ?? (root.isDestructive ? dangerAction : primaryAction)).forceActiveFocus()

    Basic.Overlay.modal: Rectangle {
        color: Theme.scrim
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
    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1
            to: 0
            duration: Theme.fast
            easing.type: Theme.easing
        }
    }

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.surface2
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: Theme.space3

        Keys.onReturnPressed: root.accepted()
        Keys.onEnterPressed: root.accepted()

        Text {
            text: root.title
            font.family: Theme.uiFamily
            font.pointSize: Theme.titleSize
            font.weight: Font.DemiBold
            color: Theme.text1
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        ColumnLayout {
            id: body
            spacing: Theme.space2
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.space2
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }
            SecondaryButton {
                text: Theme.textCancel
                onClicked: root.close()
            }
            PrimaryButton {
                id: primaryAction
                text: root.acceptText
                visible: !root.isDestructive
                onClicked: root.accepted()
            }
            DangerButton {
                id: dangerAction
                text: root.acceptText
                visible: root.isDestructive
                onClicked: root.accepted()
            }
        }
    }
}
