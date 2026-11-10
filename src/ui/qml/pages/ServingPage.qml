import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen (04 section 5, the approved layout), top to bottom, full width: controls,
// state and progress, monitor tiles, network, and the log drawer at the foot. The Parameters
// panel slides in from the right over the screen's right part under the controls row, so its
// toggle stays in reach while the tiles shrink beside it; it starts closed.
Rectangle {
    id: root

    property bool isLogOpen: false
    property bool isParametersOpen: false
    property string modelPath: LaunchController.modelPath
    property string modelName: LaunchController.modelName
    readonly property bool isLoading: LaunchController.state === LaunchController.Loading

    signal logClosed

    color: Theme.bg
    onIsParametersOpenChanged: {
        if (root.isParametersOpen) {
            PlannerRunner.modelPath = root.modelPath
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ServingControls {
            Layout.margins: Theme.space4
            Layout.bottomMargin: 0
            modelPath: root.modelPath
            modelName: root.modelName
            isParametersOpen: root.isParametersOpen
            Layout.fillWidth: true
            onModelPicked: (modelPath, modelName) => {
                root.modelPath = modelPath
                root.modelName = modelName
                if (root.isParametersOpen) {
                    PlannerRunner.modelPath = modelPath
                }
            }
            onParametersToggled: root.isParametersOpen = !root.isParametersOpen
        }

        RowLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollPage {
                // Preferred 0: the open Parameters panel takes its width from this column, not over it.
                Layout.preferredWidth: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    spacing: Theme.space2
                    Layout.fillWidth: true

                    RowLayout {
                        spacing: Theme.space3
                        Layout.fillWidth: true

                        ServerStateText {}

                        Text {
                            text: LaunchController.failureReason
                            visible: LaunchController.state === LaunchController.Failed && text !== ""
                            font.family: Theme.uiFamily
                            font.pointSize: Theme.bodySize
                            color: Theme.text2
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    UsageBar {
                        fraction: LaunchController.loadProgress < 0 ? -1 : LaunchController.loadProgress / 100
                        visible: root.isLoading
                        Layout.fillWidth: true
                    }
                }

                MonitorTiles {
                    Layout.fillWidth: true
                }

                NetworkGroup {
                    Layout.fillWidth: true
                }
            }

            ParametersPanel {
                Layout.fillHeight: true
                Layout.preferredWidth: root.isParametersOpen ? SettingsStore.parametersPanelWidth : 0
                Behavior on Layout.preferredWidth {
                    NumberAnimation {
                        duration: Theme.normal
                        easing.type: Theme.easing
                    }
                }
                visible: Layout.preferredWidth > 0
                onCloseRequested: root.isParametersOpen = false
                onResized: delta => SettingsStore.parametersPanelWidth = Math.max(Theme.parametersPanelMinWidth,
                    Math.min(SettingsStore.parametersPanelWidth + delta, root.width / 2))
            }
        }

        LogDrawer {
            Layout.fillWidth: true
            Layout.preferredHeight: root.isLogOpen ? SettingsStore.logDrawerHeight : 0
            Behavior on Layout.preferredHeight {
                NumberAnimation {
                    duration: Theme.normal
                    easing.type: Theme.easing
                }
            }
            visible: Layout.preferredHeight > 0
            onCloseRequested: root.logClosed()
            onResized: delta => SettingsStore.logDrawerHeight = Math.max(Theme.logDrawerMinHeight,
                Math.min(SettingsStore.logDrawerHeight + delta, root.height / 2))
        }
    }
}
