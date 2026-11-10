import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// The Serving screen's network row (04 section 7.2), one line with no group header: serve on the
// local network (on = 0.0.0.0, off = 127.0.0.1) and the port. Changes apply on the next start.
RowLayout {
    id: root

    spacing: Theme.space3
    Layout.preferredHeight: Theme.propertyRowHeight

    CheckRow {
        id: lanBox
        text: qsTr("Serve on local network")
        isTicked: SettingsStore.serveOnLan
        // The box lines up with the readout labels above, not with the row's hover inset.
        Layout.leftMargin: -Theme.space3
        onTickToggled: isTicked => SettingsStore.serveOnLan = isTicked

        ThemedToolTip {
            visible: lanBox.hovered
            text: qsTr("On listens on every network adapter (0.0.0.0); off serves this PC only (127.0.0.1). Applies on the next start.")
        }
    }

    Text {
        text: qsTr("Port")
        font.family: Theme.uiFamily
        font.pointSize: Theme.bodySize
        color: Theme.text2
        Layout.leftMargin: Theme.space3
    }

    NumberField {
        placeholderText: "8080"
        minimum: 1
        maximum: 65535
        text: SettingsStore.port
        fieldHeight: Theme.propertyControlHeight
        Layout.preferredWidth: Theme.columnExpertsWidth
        Layout.maximumWidth: Theme.columnExpertsWidth
        Layout.fillWidth: false
        onCommitted: value => SettingsStore.port = value.trim()
    }

    Item {
        Layout.fillWidth: true
    }
}
