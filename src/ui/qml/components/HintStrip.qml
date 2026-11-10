import QtQuick
import QtQuick.Layouts

// P1b inline hint strip: one Caption line in text3, hidden while empty; details go to tooltips.
Text {
    visible: text !== ""
    font.family: Theme.uiFamily
    font.pointSize: Theme.captionSize
    color: Theme.text3
    elide: Text.ElideRight
    Layout.fillWidth: true
}
