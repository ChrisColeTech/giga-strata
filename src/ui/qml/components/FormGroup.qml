import QtQuick
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts

// A workspace group (the family kit's form/Group.tsx): a 48px icon tile (accent-tint gradient,
// accent edge, accent glyph), a 28px mono semibold title, an optional badge; the whole heading
// folds the group and a separate 32px ghost chevron on the readout edge says which way (turned
// -90 when folded). Rows sit 12px apart; the splitter (a faded rule with an accent core and a
// soft glow) follows every group except the last. No hint under the heading, no action on it.
ColumnLayout {
    id: root

    property string title
    property string iconName
    property string badge
    property bool isLast: false
    property bool isOpen: true
    default property alias content: body.data

    function toggle() {
        root.isOpen = !root.isOpen
    }

    spacing: Theme.space4
    Accessible.role: Accessible.Grouping
    Accessible.name: title

    Item {
        implicitHeight: Theme.groupTopPadding - Theme.space4
        Layout.fillWidth: true
    }

    RowLayout {
        spacing: Theme.space3
        Layout.fillWidth: true

        Basic.AbstractButton {
            id: heading
            focusPolicy: Qt.StrongFocus
            Layout.fillWidth: true
            Accessible.role: Accessible.Button
            Accessible.name: root.isOpen ? qsTr("Collapse %1").arg(root.title) : qsTr("Expand %1").arg(root.title)
            onClicked: root.toggle()

            background: FocusRing {
                visible: heading.visualFocus
            }

            contentItem: RowLayout {
                spacing: Theme.space3

                Rectangle {
                    visible: root.iconName !== ""
                    implicitWidth: Theme.groupTileSize
                    implicitHeight: Theme.groupTileSize
                    radius: Theme.groupTileRadius
                    border.width: 1
                    border.color: Theme.tileEdge
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Theme.accentTint }
                        GradientStop { position: 1; color: "transparent" }
                    }

                    Rectangle {
                        x: parent.radius
                        y: 1
                        width: parent.width - 2 * parent.radius
                        height: 1
                        color: Theme.strokeHighlight
                    }

                    Icon {
                        anchors.centerIn: parent
                        name: root.iconName
                        size: Theme.groupTileIconSize
                        color: Theme.accentText
                    }
                }

                Text {
                    text: root.title
                    font.family: Theme.monoFamily
                    font.pixelSize: Theme.groupTitlePixelSize
                    font.weight: Font.DemiBold
                    lineHeight: Theme.groupTitleLineHeight
                    lineHeightMode: Text.FixedHeight
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.text1
                    elide: Text.ElideRight
                    Layout.maximumWidth: heading.width - Theme.groupTileSize - Theme.space3
                }

                Rectangle {
                    visible: root.badge !== ""
                    radius: Theme.radiusSm
                    color: Theme.surface2
                    implicitWidth: badgeText.implicitWidth + 2 * Theme.space2
                    implicitHeight: badgeText.implicitHeight + Theme.space1

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: root.badge
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.notePixelSize
                        color: Theme.text2
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        IconButton {
            iconName: "chevron-down"
            toolTipText: root.isOpen ? qsTr("Collapse") : qsTr("Expand")
            implicitWidth: Theme.groupChevronSize
            implicitHeight: Theme.groupChevronSize
            Accessible.name: root.isOpen ? qsTr("Collapse %1").arg(root.title) : qsTr("Expand %1").arg(root.title)
            onClicked: root.toggle()

            contentItem: Item {
                Icon {
                    anchors.centerIn: parent
                    name: "chevron-down"
                    color: Theme.text2
                    rotation: root.isOpen ? 0 : -90
                    Behavior on rotation {
                        NumberAnimation {
                            duration: Theme.groupFold
                            easing.type: Theme.easing
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: body
        visible: root.isOpen
        spacing: Theme.space3
        Layout.fillWidth: true
    }

    Item {
        visible: !root.isLast
        implicitHeight: 1
        Layout.fillWidth: true
        Layout.topMargin: Theme.splitterGap

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 1
            width: parent.width
            height: Theme.splitterGlowRadius
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.5; color: Theme.splitterGlow }
                GradientStop { position: 1; color: "transparent" }
            }
        }

        Rectangle {
            anchors.fill: parent
            opacity: Theme.splitterOpacity
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.22; color: Theme.text3 }
                GradientStop { position: 0.5; color: Theme.accent }
                GradientStop { position: 0.78; color: Theme.text3 }
                GradientStop { position: 1; color: "transparent" }
            }
        }
    }
}
