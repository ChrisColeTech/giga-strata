import QtQuick

// 6.3 Overline: the section label, ALL CAPS by definition (T3).
Text {
    font.family: Theme.uiFamily
    font.pointSize: Theme.overlineSize
    font.weight: Font.DemiBold
    font.letterSpacing: Theme.overlineTracking
    font.capitalization: Font.AllUppercase
    color: Theme.text2
    elide: Text.ElideRight
}
