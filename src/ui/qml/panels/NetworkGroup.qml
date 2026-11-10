import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's Network group (04 section 5, row 4): serve on the local network (on =
// 0.0.0.0, off = 127.0.0.1), the port, and the URL with Copy. Changes apply on the next start.
FormGroup {
    id: root

    readonly property string lanLabel: qsTr("Serve on local network")
    readonly property string portLabel: qsTr("Port")
    readonly property string urlLabel: qsTr("URL")
    readonly property real labelWidth: labelColumn.width

    title: qsTr("Network")
    iconName: "network"
    isLast: true

    LabelColumn {
        id: labelColumn
        labels: [root.lanLabel, root.portLabel, root.urlLabel]
    }

    FormRow {
        label: root.lanLabel
        labelWidth: root.labelWidth
        toolTipText: qsTr("On listens on every network adapter (0.0.0.0); off serves this PC only (127.0.0.1).")
        Layout.fillWidth: true

        Toggle {
            accessibleName: root.lanLabel
            isOn: SettingsStore.serveOnLan
            onSwitched: isOn => SettingsStore.serveOnLan = isOn
        }
    }

    FormRow {
        label: root.portLabel
        labelWidth: root.labelWidth
        Layout.fillWidth: true

        NumberField {
            placeholderText: "8080"
            minimum: 1
            maximum: 65535
            text: SettingsStore.port
            Layout.preferredWidth: Theme.columnSizeWidth
            Layout.maximumWidth: Theme.columnSizeWidth
            Layout.fillWidth: false
            onCommitted: value => SettingsStore.port = value.trim()
        }
    }

    FormRow {
        label: root.urlLabel
        labelWidth: root.labelWidth
        Layout.fillWidth: true

        Text {
            text: LaunchController.url === "" ? "—" : LaunchController.url
            font.family: Theme.monoFamily
            font.pointSize: Theme.bodySize
            color: LaunchController.url === "" ? Theme.text3 : Theme.text1
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        GhostButton {
            text: qsTr("Copy")
            enabled: LaunchController.url !== ""
            toolTipText: LaunchController.url !== "" ? qsTr("Copy the URL") : qsTr("The URL exists while the server is serving.")
            onClicked: LaunchController.copyUrl()
        }
    }
}
