import QtQuick
import QtQuick.Controls.Basic as Basic

// S5 separator: a 1px border line between groups of menu items.
Basic.MenuSeparator {
    topPadding: Theme.space1
    bottomPadding: Theme.space1
    leftPadding: Theme.space2
    rightPadding: Theme.space2

    contentItem: Rectangle {
        implicitHeight: 1
        color: Theme.border
    }
}
