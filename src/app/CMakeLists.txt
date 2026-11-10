# giga_app: C++ controllers exposed to QML as the GigaStrata.App module (R9 singletons).
# QML_ELEMENT types must be compiled inside a QML module target, so this library is one.
# QLocalServer (single instance) lives in Qt Network, which only this layer needs.
find_package(Qt6 6.8 REQUIRED COMPONENTS Network)

qt_add_library(giga_app STATIC)
qt_add_qml_module(giga_app
    URI GigaStrata.App
    VERSION 1.0
    SOURCES
        system_info.h system_info.cpp
        settings_file.h settings_file.cpp
        settings_store.h settings_store.cpp
        monitor_controller.h monitor_controller.cpp
        tray_host.h tray_host.cpp
        single_instance.h single_instance.cpp
        # Track D: the 05 controllers and their helpers
        log_model.h log_model.cpp
        model_config_store.h model_config_store.cpp
        server_build.h server_build.cpp
        plan_tiers.h plan_tiers.cpp
        plan_job.h plan_job.cpp
        planner_runner.h planner_runner.cpp
        model_entry.h
        model_scanner.h model_scanner.cpp
        model_table_model.h model_table_model.cpp
        model_table_proxy.h model_table_proxy.cpp
        shell_actions.h shell_actions.cpp
        model_library.h model_library.cpp
        server_process.h server_process.cpp
        launch_preparer.h launch_preparer.cpp
        launch_controller.h launch_controller.cpp
)
# The app icon, served as :/giga-strata/app-icon.svg for the window and tray.
qt_add_resources(giga_app "app_icon"
    PREFIX "/giga-strata"
    BASE ${PROJECT_SOURCE_DIR}/assets
    FILES ${PROJECT_SOURCE_DIR}/assets/app-icon.svg
)
target_include_directories(giga_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(giga_app PUBLIC giga_core Qt6::Gui Qt6::Qml Qt6::Concurrent Qt6::Widgets Qt6::Network
                               PRIVATE Qt6::Svg dwmapi)
giga_strata_warnings(giga_app)
