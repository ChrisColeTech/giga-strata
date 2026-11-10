import QtQuick
import QtQuick.Layouts

// The 1px border rule between the blocks of a dense surface (a header and its body, the Serving
// screen's rows, a panel's footer).
Rectangle {
    color: Theme.border
    implicitHeight: 1
    Layout.fillWidth: true
}
