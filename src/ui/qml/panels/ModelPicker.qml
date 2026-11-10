pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's model picker (04 section 5, `▼ <model>`): a field-shaped trigger showing
// the chosen model; its popup lists the library's models (single choice, a press picks and
// closes). The page owns the choice.
Basic.AbstractButton {
    id: root

    property string modelPath
    property string modelName

    signal picked(string modelPath, string modelName)

    implicitHeight: Theme.controlHeight
    implicitWidth: Theme.modelPickerWidth
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.ComboBox
    Accessible.name: qsTr("Model")
    Accessible.description: modelName

    onClicked: list.open()

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.field
        border.width: 1
        border.color: list.visible ? Theme.accent : (root.hovered ? Theme.border2 : Theme.border)

        FocusRing {
            visible: root.visualFocus
        }
    }

    contentItem: RowLayout {
        spacing: Theme.space2

        Icon {
            name: "chevron-down"
            color: Theme.text2
            Layout.leftMargin: Theme.space2
        }

        Text {
            text: root.modelName === "" ? qsTr("Choose a model") : root.modelName
            font.family: Theme.uiFamily
            font.pointSize: Theme.bodySize
            color: root.modelName === "" ? Theme.text3 : Theme.text1
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    Basic.Popup {
        id: list
        y: root.height + Theme.space1
        width: root.width
        height: Math.min(rows.contentHeight + 2 * padding, Theme.workLeastBody)
        padding: Theme.space1
        focus: true

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.surface3
            border.width: 1
            border.color: Theme.border2
        }

        contentItem: ListView {
            id: rows
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: ModelLibrary.allModels
            delegate: Basic.AbstractButton {
                id: choice
                required property string name
                required property string path

                width: rows.width
                implicitHeight: Theme.menuItemHeight
                Accessible.role: Accessible.ListItem
                Accessible.name: name
                onClicked: choice.pick()
                Accessible.onPressAction: choice.pick()

                function pick() {
                    list.close()
                    root.picked(choice.path, choice.name)
                }

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: choice.hovered ? Theme.surface2 : "transparent"
                }

                contentItem: RowLayout {
                    spacing: Theme.space2

                    Icon {
                        name: "check"
                        color: Theme.accent
                        opacity: choice.path === root.modelPath ? 1 : 0
                        Layout.leftMargin: Theme.space2
                    }

                    Text {
                        text: choice.name
                        font.family: Theme.uiFamily
                        font.pointSize: Theme.bodySize
                        color: Theme.text1
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
