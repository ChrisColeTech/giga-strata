import QtQuick
import QtQuick.Layouts

// A read-only form row: a label and a mono value. A fact nothing reads yet shows a dash and
// says why in its tooltip (`disabledReason`).
FormRow {
    id: root

    property string value

    Layout.fillWidth: true

    Text {
        text: root.value === "" ? "—" : root.value
        font.family: Theme.monoFamily
        font.pointSize: Theme.bodySize
        color: Theme.text1
        elide: Text.ElideRight
        Layout.fillWidth: true
    }
}
