import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import GigaStrata.App

// Window shell (04 sections 1 and 7.1): the menu bar, the rail of the two screens, the active
// screen, the status bar.
// Created hidden; main.cpp applies the dark title bar, then shows it.
ApplicationWindow {
    id: window

    readonly property int servingScreen: 1

    width: Theme.windowWidth
    height: Theme.windowHeight
    minimumWidth: Theme.windowMinWidth
    minimumHeight: Theme.windowMinHeight
    visible: false
    title: Theme.appName
    color: Theme.bg

    menuBar: AppMenuBar {
        id: appMenuBar
        onModelFoldersRequested: window.openSettings(settingsWindow.modelsSection)
        onSettingsRequested: window.openWindow(settingsWindow, window.activeFocusItem)
        onExitRequested: window.exit()
        onAboutRequested: window.openWindow(aboutWindow, window.activeFocusItem)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            NavRail {
                id: navRail
                Layout.fillHeight: true
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: navRail.currentIndex

                ModelsPage {
                    onConfigRequested: (path, name, opener) => window.openModelConfig(path, name, opener)
                    onAddFolderRequested: opener => {
                        window.closeOthers(settingsWindow)
                        settingsWindow.showSection(settingsWindow.modelsSection, opener)
                    }
                }
                ServingPage {
                    id: servingPage
                    isLogOpen: SettingsStore.isLogOpen
                    onLogClosed: SettingsStore.isLogOpen = false
                }
            }
        }

        StatusBar {
            isLogOpen: SettingsStore.isLogOpen
            Layout.fillWidth: true
            // The drawer belongs to the Serving screen, so opening it shows that screen.
            onLogToggled: {
                SettingsStore.isLogOpen = !SettingsStore.isLogOpen
                if (SettingsStore.isLogOpen) {
                    navRail.currentIndex = window.servingScreen
                }
            }
        }
    }

    // One child window at a time (04 section 3): opening one closes any other.
    function closeOthers(childWindow: ChildWindow) {
        for (const other of [settingsWindow, modelConfigWindow, aboutWindow]) {
            if (other !== childWindow) {
                other.close()
            }
        }
    }

    function openWindow(childWindow: ChildWindow, opener: Item) {
        window.closeOthers(childWindow)
        childWindow.show(opener)
    }

    function openModelConfig(path: string, name: string, opener: Item) {
        window.closeOthers(modelConfigWindow)
        modelConfigWindow.showModel(path, name, opener)
    }

    function openSettings(sectionIndex: int) {
        window.closeOthers(settingsWindow)
        settingsWindow.showSection(sectionIndex, window.activeFocusItem)
    }

    // Exit is Quit in the tray: the server stops first, then the app quits.
    function exit() {
        LaunchController.stop()
        Qt.quit()
    }

    Shortcut {
        sequence: "Ctrl+,"
        onActivated: window.openWindow(settingsWindow, window.activeFocusItem)
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: window.exit()
    }

    AboutWindow {
        id: aboutWindow
    }

    SettingsWindow {
        id: settingsWindow
    }

    // PlannerRunner edits one model at a time: when the config window lets go, the Parameters
    // panel gets its model back.
    ModelConfigWindow {
        id: modelConfigWindow
        onReleased: {
            if (servingPage.isParametersOpen) {
                PlannerRunner.modelPath = servingPage.modelPath
            }
        }
    }
}
