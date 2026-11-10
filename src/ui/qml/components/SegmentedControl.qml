pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic as Basic

// S2: small enumerated choice. Controlled: `currentIndex` comes from the owner, a click or an
// arrow key emits `activated(index)` and the owner writes the setting back.
Basic.Control {
    id: root

    property var model: []
    property int currentIndex: -1
    property string toolTipText
    // A property-grid or log-header row sets it for the 24px items of 04 section 7.
    property bool isCompact: false
    readonly property int itemHeight: isCompact ? Theme.propertyControlHeight : Theme.segmentItemHeight
    readonly property int itemPadding: isCompact ? Theme.space2 : Theme.space3

    signal activated(int index)

    implicitHeight: itemHeight + 2 * padding
    implicitWidth: segments.implicitWidth + 2 * padding
    padding: 2
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.PageTabList

    Keys.onLeftPressed: root.step(-1)
    Keys.onRightPressed: root.step(1)

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.surface2

        FocusRing {
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: segments.implicitWidth
        implicitHeight: root.itemHeight

        Rectangle {
            readonly property Item selectedItem: segments.children[root.currentIndex] ?? null

            visible: selectedItem !== null
            x: selectedItem ? selectedItem.x : 0
            width: selectedItem ? selectedItem.width : 0
            height: parent.height
            radius: Theme.radiusSm
            color: Theme.surface3
            Behavior on x {
                NumberAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }
            Behavior on width {
                NumberAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }
        }

        Row {
            id: segments
            height: parent.height

            Repeater {
                model: root.model
                delegate: Basic.AbstractButton {
                    id: segment
                    required property string modelData
                    required property int index

                    readonly property bool isSelected: index === root.currentIndex

                    width: Math.max(root.itemHeight, label.implicitWidth + 2 * root.itemPadding)
                    height: root.itemHeight
                    focusPolicy: Qt.NoFocus
                    Accessible.name: modelData
                    onClicked: root.activated(index)

                    contentItem: Text {
                        id: label
                        text: segment.modelData
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: Theme.uiFamily
                        font.pointSize: Theme.bodySize
                        color: segment.isSelected || segment.hovered ? Theme.text1 : Theme.text2
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.fast
                                easing.type: Theme.easing
                            }
                        }
                    }
                }
            }
        }
    }

    ThemedToolTip {
        visible: root.hovered && root.toolTipText !== ""
        text: root.toolTipText
    }

    function step(delta) {
        const next = Math.max(0, Math.min(root.model.length - 1, root.currentIndex + delta))
        if (next !== root.currentIndex) {
            root.activated(next)
        }
    }
}
