import QtQuick
import QtQuick.Layouts

// 6.3 Label role: the name above a field or a control group (F2).
Text {
    font.family: Theme.uiFamily
    font.pointSize: Theme.bodySize
    font.weight: Font.DemiBold
    color: Theme.text2
    elide: Text.ElideRight
    Layout.fillWidth: true
}
