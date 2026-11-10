pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic

// P1c toasts: bottom-center pills (surface3 + border, text1, icon in the kind's semantic color),
// newest at the bottom, at most toastStackLimit, auto-dismiss after toastDuration, click to
// dismiss. Kinds: "info", "success", "error".
Item {
    id: root

    function show(message, kind) {
        if (toasts.count >= Theme.toastStackLimit) {
            toasts.remove(0)
        }
        toasts.append({ message: message, kind: kind || "info" })
    }

    ListModel {
        id: toasts
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space4
        spacing: Theme.space2

        Repeater {
            model: toasts
            delegate: Basic.AbstractButton {
                id: toast
                required property string message
                required property string kind
                required property int index

                readonly property color kindColor: kind === "error" ? Theme.danger
                    : (kind === "success" ? Theme.success : Theme.info)

                anchors.horizontalCenter: parent.horizontalCenter
                implicitHeight: Theme.controlHeight
                implicitWidth: Math.min(root.width - 2 * Theme.space4, row.implicitWidth + 2 * Theme.space4)
                Accessible.role: Accessible.AlertMessage
                Accessible.name: message
                onClicked: toasts.remove(index)

                opacity: 0
                Component.onCompleted: opacity = 1
                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.normal
                        easing.type: Theme.easing
                    }
                }

                Timer {
                    interval: Theme.toastDuration
                    running: true
                    onTriggered: toasts.remove(toast.index)
                }

                background: Rectangle {
                    radius: height / 2
                    color: Theme.surface3
                    border.width: 1
                    border.color: Theme.border
                }

                contentItem: Item {
                    Row {
                        id: row
                        anchors.centerIn: parent
                        spacing: Theme.space2

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: toast.kind === "error" ? "triangle-alert" : (toast.kind === "success" ? "check" : "info")
                            color: toast.kindColor
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(implicitWidth, root.width - 2 * Theme.space6 - Theme.iconSize)
                            text: toast.message
                            elide: Text.ElideRight
                            font.family: Theme.uiFamily
                            font.pointSize: Theme.bodySize
                            color: Theme.text1
                        }
                    }
                }
            }
        }
    }
}
