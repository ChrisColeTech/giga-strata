pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

// Settings > About (04 section 3): the version, the bundled server's fork commit, licences.
Workspace {
    id: root

    property real labelWidth
    readonly property var versionRows: [
        { label: qsTr("Version"), value: Qt.application.version, reason: "" },
        { label: qsTr("Server build"), value: "", reason: qsTr("The bundled server's build info is not read yet.") }
    ]
    readonly property var licenceRows: [
        { label: qsTr("Qt"), value: qsTr("LGPL v3"), reason: "" },
        { label: qsTr("llama.cpp"), value: qsTr("MIT"), reason: "" },
        { label: qsTr("Lucide icons"), value: qsTr("ISC"), reason: "" },
        { label: qsTr("VS Code codicons"), value: qsTr("CC BY 4.0"), reason: "" }
    ]
    readonly property var labels: versionRows.concat(licenceRows).map(row => row.label)

    FormGroup {
        title: qsTr("Giga Strata")
        iconName: "info"
        Layout.fillWidth: true

        Repeater {
            model: root.versionRows
            delegate: FactRow {
                required property var modelData
                label: modelData.label
                value: modelData.value
                disabledReason: modelData.reason
                labelWidth: root.labelWidth
            }
        }
    }

    FormGroup {
        title: qsTr("Licences")
        iconName: "scale"
        isLast: true
        Layout.fillWidth: true

        Repeater {
            model: root.licenceRows
            delegate: FactRow {
                required property var modelData
                label: modelData.label
                value: modelData.value
                labelWidth: root.labelWidth
            }
        }
    }
}
