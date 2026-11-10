pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// Help > About Giga Strata (04 section 7.1): a small child window with the version, the bundled
// server's fork commit and branch (server\BUILD_INFO.txt), the licences and the repository link.
ChildWindow {
    id: root

    // The licence names are the longest labels here.
    readonly property int labelWidth: Theme.gaugeLabelWidth
    readonly property bool hasBuildInfo: AboutInfo.serverCommit !== ""
    readonly property var licenceRows: [
        { label: qsTr("Qt"), value: qsTr("LGPL v3") },
        { label: qsTr("llama.cpp"), value: qsTr("MIT") },
        { label: qsTr("Lucide icons"), value: qsTr("ISC") },
        { label: qsTr("VS Code codicons"), value: qsTr("CC BY 4.0") }
    ]

    title: qsTr("About %1").arg(Theme.appName)
    specWidth: Theme.dialogWidth
    specBody: Theme.aboutBody
    specMinWidth: Theme.dialogWidth
    specMinBody: Theme.aboutBody
    isMaximizable: false

    Rectangle {
        anchors.fill: parent
        color: Theme.workspace

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.space4
            spacing: 0

            Text {
                text: Theme.appName
                font.family: Theme.uiFamily
                font.pointSize: Theme.titleSize
                font.weight: Font.DemiBold
                color: Theme.text1
                Layout.bottomMargin: Theme.space2
            }

            PropertyRow {
                label: qsTr("Version")
                labelWidth: root.labelWidth

                Text {
                    text: Qt.application.version
                    font.family: Theme.monoFamily
                    font.pointSize: Theme.bodySize
                    color: Theme.text1
                }
            }

            PropertyRow {
                label: qsTr("Server build")
                labelWidth: root.labelWidth
                status: root.hasBuildInfo ? AboutInfo.serverBranch : ""
                disabledReason: root.hasBuildInfo ? "" : qsTr("No server\\BUILD_INFO.txt beside the app: this build has no bundled server.")

                Text {
                    text: root.hasBuildInfo ? AboutInfo.serverCommit.substring(0, 12) : "—"
                    font.family: Theme.monoFamily
                    font.pointSize: Theme.bodySize
                    color: Theme.text1
                }
            }

            PropertyRow {
                label: qsTr("Repository")
                labelWidth: root.labelWidth
                isControlWide: true

                Text {
                    text: "<a href=\"%1\">%2</a>".arg(Theme.repositoryUrl).arg(Theme.repositoryUrl.replace("https://", ""))
                    textFormat: Text.StyledText
                    linkColor: Theme.accent
                    font.family: Theme.uiFamily
                    font.pointSize: Theme.bodySize
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    onLinkActivated: link => Qt.openUrlExternally(link)

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            PropertySection {
                title: qsTr("Licences")
                Layout.topMargin: Theme.space2

                Repeater {
                    model: root.licenceRows
                    delegate: PropertyRow {
                        id: licenceRow
                        required property var modelData
                        label: modelData.label
                        labelWidth: root.labelWidth

                        Text {
                            text: licenceRow.modelData.value
                            font.family: Theme.monoFamily
                            font.pointSize: Theme.bodySize
                            color: Theme.text1
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
