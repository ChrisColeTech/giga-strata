pragma Singleton

import QtQuick
import GigaStrata.App

// Every color, font, size and duration in the app (01 section 6.9). Names mirror the guide
// one to one; add a token there before adding it here.
QtObject {
    // 6.2 surfaces and structure
    readonly property color bg: "#15171C"
    readonly property color surface1: "#1B1E25"
    readonly property color surface2: "#222630"
    readonly property color surface3: "#2A2F3B"
    readonly property color field: "#101216"
    readonly property color border: "#2E3440"
    readonly property color border2: "#3B4252"

    // 6.2 text
    readonly property color text1: "#E4E8F0"
    readonly property color text2: "#9AA4B5"
    readonly property color text3: "#626C7E"
    readonly property color textOnAccent: "#0E1116"
    readonly property color textOnDanger: "#16080C"
    readonly property color knob: "#F2F4F8"
    readonly property color scrim: Qt.rgba(0, 0, 0, 0.5)
    readonly property color focusRing: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.55)
    readonly property color glowAccent: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.20)

    // 6.2 identity and tiers
    readonly property color accent: "#4FC1A8"
    readonly property color accentSoft: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.16)
    readonly property color accentHover: Qt.tint(accent, Qt.rgba(1, 1, 1, 0.08))
    readonly property color accentPressed: Qt.tint(accent, Qt.rgba(0, 0, 0, 0.08))
    readonly property var tierGpu: ["#7E8FEF", "#B08AF0", "#E57FC4", "#45C9E0"]
    readonly property color tierRam: "#E0B84E"
    readonly property color tierSsd: "#7D879C"

    // 6.2 semantics
    readonly property color success: "#5EC97F"
    readonly property color warn: "#E2804E"
    readonly property color danger: "#E5556A"
    readonly property color info: "#6FA8E8"
    readonly property color dangerHover: Qt.tint(danger, Qt.rgba(1, 1, 1, 0.08))
    readonly property color dangerPressed: Qt.tint(danger, Qt.rgba(0, 0, 0, 0.08))

    // 6.3 typography
    readonly property string uiFamily: "Segoe UI"
    readonly property string monoFamily: SystemInfo.monoFamily
    readonly property real displaySize: 20
    readonly property real titleSize: 14
    readonly property real overlineSize: 10
    readonly property real bodySize: 10.5
    readonly property real captionSize: 9.5
    readonly property real overlineTracking: 0.8

    // 6.4 space, shape, density
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 24
    readonly property int space6: 32
    readonly property int radiusSm: 4
    readonly property int radiusMd: 6
    readonly property int radiusLg: 8
    readonly property int controlHeight: 30
    readonly property int iconSize: 16
    readonly property int railWidth: 64
    readonly property int railItemHeight: 48
    readonly property int railIconSize: 20
    readonly property int railCaptionSize: 9
    readonly property int railIndicatorWidth: 3
    readonly property int statusBarHeight: 36
    readonly property int splitHitWidth: 6
    readonly property int focusRingWidth: 2
    readonly property int pageMinWidth: 380
    readonly property int monitorMinWidth: 360
    readonly property int windowWidth: 1280
    readonly property int windowHeight: 800
    readonly property int windowMinWidth: 960
    readonly property int windowMinHeight: 640
    readonly property int buttonPadding: 16
    readonly property int chipHeight: 26
    readonly property int segmentItemHeight: 28
    readonly property int switchWidth: 36
    readonly property int switchHeight: 20
    readonly property int switchKnobSize: 14
    readonly property int menuItemHeight: 28
    readonly property int menuMinWidth: 200
    readonly property int dotSize: 6
    readonly property int barHeight: 4
    readonly property int gaugeRowHeight: 24
    readonly property int gaugeLabelWidth: 120
    readonly property int scrollBarWidth: 6
    readonly property int dialogWidth: 420
    readonly property int presetCardWidth: 220
    readonly property real disabledOpacity: 0.35
    readonly property int toolTipDelay: 500
    readonly property int toastDuration: 6000
    readonly property int toastStackLimit: 3

    // 6.4 child windows: the family kit's child-metrics.ts and SettingsRail.tsx, ported
    readonly property real childOpenShare: 0.8
    readonly property int childGrip: 5
    readonly property int childBorder: 1
    readonly property int childTitleHeight: 28
    readonly property int childHeaderGap: 6
    readonly property int childInset: 8
    readonly property int workWidth: 1200
    readonly property int workBody: 840
    readonly property int workLeastWidth: 640
    readonly property int workLeastBody: 360
    readonly property int captionIconSize: 15
    readonly property color shadowMenu: Qt.rgba(0, 0, 0, 0.28)
    readonly property int shadowMenuOffset: 8
    readonly property real shadowMenuBlur: 0.5
    readonly property color accentTint: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.12)
    readonly property color accentEdge: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.40)

    // 6.4 the family form kit (form/Group.tsx, Row.tsx, Workspace.tsx, Actions.tsx, InfoTip.tsx,
    // form-classes.ts), ported: Tailwind values in px
    readonly property color workspace: "#0F1115"
    readonly property color accentText: "#84D4C2"
    readonly property color tileEdge: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.16)
    readonly property color strokeHighlight: Qt.rgba(1, 1, 1, 0.05)
    readonly property color splitterGlow: Qt.rgba(79 / 255, 193 / 255, 168 / 255, 0.08)
    readonly property real splitterOpacity: 0.6
    readonly property int workspacePadding: 16
    readonly property int workspaceBottom: 8
    readonly property int groupTopPadding: 20
    readonly property int groupTileSize: 48
    readonly property int groupTileRadius: 12
    readonly property int groupTileIconSize: 20
    readonly property int groupTitlePixelSize: 28
    readonly property int groupTitleLineHeight: 36
    readonly property int groupChevronSize: 32
    readonly property int splitterGap: 20
    readonly property int splitterGlowRadius: 14
    readonly property int labelPixelSize: 14
    readonly property int notePixelSize: 12
    readonly property int infoTipSize: 20
    readonly property int rowGap: 6
    readonly property int rowAccentWidth: 4
    readonly property color rowInactive: Qt.rgba(98 / 255, 108 / 255, 126 / 255, 0.4)
    readonly property int fieldHeight: 36
    readonly property int settingsRailWidth: 150
    readonly property int settingsRailWideWidth: 210
    readonly property int settingsRailWideAt: 1024
    readonly property int settingsRailPadding: 6
    readonly property int railRowHeight: 34
    readonly property int railEdgeWidth: 2
    readonly property int labelCharWidth: 8
    readonly property int labelColumnMin: 120
    readonly property int labelColumnGlyph: 36
    readonly property int resizeGripSize: 6
    readonly property int armTimeout: 4000
    readonly property int statusMeterWidth: 56

    // 6.4 the Models table (family record table: 36px rows, 29px header) and the Serving screen
    readonly property int tableRowHeight: 36
    readonly property int tableHeaderHeight: 29
    readonly property int columnNameMinWidth: 200
    readonly property int columnTypeWidth: 120
    readonly property int columnQuantWidth: 96
    readonly property int columnSizeWidth: 96
    readonly property int columnExpertsWidth: 80
    readonly property int columnFilesWidth: 96
    readonly property int columnModifiedWidth: 144
    readonly property int columnActionsWidth: 168
    readonly property int filterPopupWidth: 240
    readonly property int modelPickerWidth: 320
    readonly property int modelPickerMinWidth: 140
    readonly property int logDrawerMinHeight: 96
    readonly property int logLineHeight: 18
    readonly property int parametersPanelMinWidth: 400

    // 6.4 desktop density (04 section 7): the menu bar, the property grid, the readout block
    readonly property int menuBarHeight: 28
    readonly property int aboutBody: 300
    readonly property int propertyRowHeight: 28
    readonly property int propertyControlHeight: 24
    readonly property int propertyLabelWidth: 104
    readonly property int propertyLabelWideWidth: 200
    readonly property int readoutLabelWidth: 56

    // 6.6 motion
    // The Settings preference (04 section 3) wins over Windows only when it says "on".
    readonly property real motionScale: SettingsStore.reduceMotion === "on" ? 0 : SystemInfo.motionScale
    readonly property int fast: 120 * motionScale
    readonly property int normal: 180 * motionScale
    readonly property int slow: 300 * motionScale
    readonly property int switchSlide: 150 * motionScale
    readonly property int easing: Easing.OutCubic
    readonly property int indeterminateCycle: 1800
    readonly property int childEnter: 200 * motionScale
    readonly property int groupFold: 200 * motionScale
    readonly property int childRise: 10
    readonly property int childSink: 6

    // R17 shared vocabulary
    readonly property string appName: qsTr("Giga Strata")
    readonly property string repositoryUrl: "https://github.com/ChrisColeTech/giga-strata"
    readonly property string textCancel: qsTr("Cancel")
    readonly property string textBrowse: qsTr("Browse")
}
