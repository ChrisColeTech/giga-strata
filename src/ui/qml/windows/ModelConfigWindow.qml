import QtQuick
import QtQuick.Layouts
import GigaStrata.App

// One model's Model config window (04 sections 4 and 7.3), a work surface titled with the model's
// name: an info line over the model's property grid (memory bars, MAIN, EXPERT USAGE, ADVANCED)
// and its footer with Save. No Cancel: the X and Escape close it, and closing commits nothing.
ChildWindow {
    id: root

    readonly property var info: PlannerRunner.modelInfo

    signal released

    function showModel(modelPath: string, modelName: string, openerItem: Item) {
        PlannerRunner.modelPath = modelPath
        root.title = modelName
        root.show(openerItem)
    }

    function infoLine(): string {
        const sizeGb = (Number(root.info.sizeBytes ?? 0) / (1024 * 1024 * 1024)).toFixed(1)
        const files = Number(root.info.files ?? 1)
        const parts = [
            String(root.info.type ?? ""),
            qsTr("%1 layers").arg(Number(root.info.layers ?? 0)),
            Number(root.info.experts ?? 0) > 0 ? qsTr("%1 experts").arg(Number(root.info.experts)) : qsTr("dense"),
            qsTr("%1 GB").arg(sizeGb),
            files === 1 ? qsTr("1 file") : qsTr("%1 files").arg(files),
            String(root.info.draftPath ?? "") !== "" ? qsTr("MTP draft found") : qsTr("no MTP draft"),
            String(root.info.projectorPath ?? "") !== "" ? qsTr("vision projector found") : qsTr("no vision projector")
        ]
        return parts.filter(part => part !== "").join("  ·  ")
    }

    onClosed: root.released()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            text: root.infoLine()
            topPadding: Theme.space2
            bottomPadding: Theme.space2
            leftPadding: Theme.workspacePadding
            rightPadding: Theme.workspacePadding
            font.family: Theme.monoFamily
            font.pointSize: Theme.captionSize
            color: Theme.text2
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        HairLine {}

        ParameterForm {
            commitText: qsTr("Save")
            commitToolTip: qsTr("Saves these parameters to the model's config file.")
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCommitRequested: PlannerRunner.save()
        }
    }
}
