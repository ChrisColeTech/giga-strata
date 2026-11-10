import QtQuick

// F5 number field: mono, right-aligned, validated on commit. `allowAuto` also accepts "auto" or
// an empty field, which the Python launcher reads as auto (micro-batch). Without a `maximum`
// the rule sentence names only the lower bound.
TextField {
    id: root

    property real minimum: 0
    property real maximum: Infinity
    property bool allowAuto: false

    isMono: true
    isNumeric: true

    function validate(value: string): string {
        const trimmed = value.trim()
        if (root.allowAuto && (trimmed === "" || trimmed.toLowerCase() === "auto")) {
            return ""
        }
        const isWhole = /^[0-9]+$/.test(trimmed)
        if (isWhole && Number(trimmed) >= root.minimum && Number(trimmed) <= root.maximum) {
            return ""
        }
        return root.ruleSentence()
    }

    function ruleSentence() {
        const hasMaximum = isFinite(root.maximum)
        if (root.allowAuto) {
            return hasMaximum ? qsTr("Enter auto or a whole number from %1 to %2.").arg(root.minimum).arg(root.maximum)
                              : qsTr("Enter auto or a whole number of %1 or more.").arg(root.minimum)
        }
        return hasMaximum ? qsTr("Enter a whole number from %1 to %2.").arg(root.minimum).arg(root.maximum)
                          : qsTr("Enter a whole number of %1 or more.").arg(root.minimum)
    }
}
