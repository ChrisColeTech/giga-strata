import QtQuick

// The width of a form's one label column (the family kit's lib/label-column.ts): sized from the
// longest label the window can show plus its info glyph, never under labelColumnMin, so it stays
// put as a rail changes section.
QtObject {
    property var labels: []
    readonly property real width: {
        let longest = 0
        for (const label of labels) {
            longest = Math.max(longest, label.length)
        }
        return Math.max(Theme.labelColumnMin, Math.ceil(longest * Theme.labelCharWidth) + Theme.labelColumnGlyph)
    }
}
