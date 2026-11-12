# The QML module (URI GigaStrata) as a static library; the exe links its plugin.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Lucide icons (ISC, assets/icons/LICENSE), served as qrc:/qt/qml/GigaStrata/icons/<name>.svg
set(icon_names settings info play square folder triangle-alert check x chevron-down layers server folder-plus
               chrome-maximize chrome-restore chrome-close palette app-window folder-open server-cog scale
               sliders-horizontal activity wrench network arrow-up arrow-down filter filter-filled ellipsis
               circle-stop refresh-cw panel-right rotate-ccw folder-minus copy)
set(icon_files "")
foreach(icon_name IN LISTS icon_names)
    set(icon_file ${PROJECT_SOURCE_DIR}/assets/icons/${icon_name}.svg)
    set_source_files_properties(${icon_file} PROPERTIES QT_RESOURCE_ALIAS icons/${icon_name}.svg)
    list(APPEND icon_files ${icon_file})
endforeach()

qt_add_library(giga_ui STATIC)
qt_add_qml_module(giga_ui
    URI GigaStrata
    VERSION 1.0
    QML_FILES
        qml/Theme.qml
        qml/Main.qml
        qml/components/Icon.qml
        qml/components/FocusRing.qml
        qml/components/ThemedToolTip.qml
        qml/components/ButtonBase.qml
        qml/components/GhostButton.qml
        qml/components/IconButton.qml
        qml/components/TextField.qml
        qml/components/NumberField.qml
        qml/components/PathField.qml
        qml/components/SegmentedControl.qml
        qml/components/Overline.qml
        qml/components/FieldLabel.qml
        qml/components/PrimaryButton.qml
        qml/components/SecondaryButton.qml
        qml/components/ContextMenu.qml
        qml/components/ContextMenuItem.qml
        qml/components/ToastCenter.qml
        qml/components/ChildWindow.qml
        qml/components/WindowHeader.qml
        qml/components/ResizeGrip.qml
        qml/components/SettingsRail.qml
        qml/components/ChildBounds.qml
        qml/components/CheckRow.qml
        qml/components/ArmedMenuItem.qml
        qml/components/EdgeSizer.qml
        qml/components/Workspace.qml
        qml/components/ServerStateText.qml
        qml/components/Toggle.qml
        qml/components/UsageBar.qml
        qml/panels/NavRail.qml
        qml/panels/NavRailItem.qml
        qml/panels/StatusBar.qml
        qml/panels/StatusVram.qml
        qml/panels/ModelHeaderCell.qml
        qml/panels/FilterPopup.qml
        qml/panels/ModelRow.qml
        qml/panels/ModelTable.qml
        qml/panels/GpuHeadroomFields.qml
        qml/panels/ParametersMain.qml
        qml/panels/ParametersExperts.qml
        qml/panels/ParametersAdvanced.qml
        qml/panels/ParameterForm.qml
        qml/panels/MemorySplit.qml
        qml/panels/ModelPicker.qml
        qml/panels/ServingControls.qml
        qml/panels/LogDrawer.qml
        qml/panels/ParametersPanel.qml
        qml/pages/ModelsPage.qml
        qml/pages/ServingPage.qml
        qml/panels/SettingsGeneral.qml
        qml/panels/SettingsModels.qml
        qml/panels/SettingsServer.qml
        qml/windows/SettingsWindow.qml
        qml/windows/ModelConfigWindow.qml
        qml/windows/AboutWindow.qml
        qml/panels/AppMenuBar.qml
        qml/components/MenuDivider.qml
        qml/components/PropertyRow.qml
        qml/components/PropertySection.qml
        qml/components/MeterRow.qml
        qml/components/HairLine.qml
        qml/panels/ServingReadout.qml
        qml/panels/NetworkRow.qml
        qml/panels/GpuChecks.qml
    RESOURCES
        ${icon_files}
)
target_link_libraries(giga_ui PRIVATE giga_app Qt6::Quick Qt6::QuickControls2)
