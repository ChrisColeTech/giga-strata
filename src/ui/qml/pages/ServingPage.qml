import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen, compact (04 sections 5 and 7.2), top to bottom: the controls row with the
// state, the readout block, the network row, and the log taking the remaining height. The
// Parameters panel slides in from the right under the controls row, beside all three; it starts
// closed. The log's top edge resizes it: dragged down it keeps that height and leaves the rest
// empty, dragged back to the network row it fills again (a stored height of 0 means fill).
Rectangle {
    id: root

    property bool isLogOpen: false
    property bool isParametersOpen: false
    property string modelPath: LaunchController.modelPath
    property string modelName: LaunchController.modelName
    readonly property bool isLogFilling: SettingsStore.logDrawerHeight <= 0

    signal logClosed

    function resizeLog(delta: real) {
        const room = logDrawer.height + logSlack.height
        const next = logDrawer.height + delta
        SettingsStore.logDrawerHeight = next >= room - 1 ? 0 : Math.max(Theme.logDrawerMinHeight, next)
    }

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
            Layout.margins: Theme.space2
            Layout.leftMargin: Theme.space4
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

        HairLine {}

        RowLayout {
            spacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                spacing: 0
                // Preferred 0: the open Parameters panel takes its width from this column, not over it.
                Layout.preferredWidth: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                ServingReadout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.space4
                    Layout.rightMargin: Theme.space4
                    Layout.topMargin: Theme.space1
                    Layout.bottomMargin: Theme.space1
                }

                HairLine {}

                NetworkRow {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.space4
                    Layout.rightMargin: Theme.space4
                    Layout.topMargin: Theme.space1
                    Layout.bottomMargin: Theme.space1
                }

                Item {
                    id: logSlack
                    visible: !root.isLogOpen || !root.isLogFilling
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                LogDrawer {
                    id: logDrawer
                    visible: root.isLogOpen
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: Theme.logDrawerMinHeight
                    Layout.maximumHeight: root.isLogFilling ? Number.POSITIVE_INFINITY : SettingsStore.logDrawerHeight
                    onCloseRequested: root.logClosed()
                    onResized: delta => root.resizeLog(delta)
                }
            }

            ParametersPanel {
                modelName: root.modelName
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
    }
}
