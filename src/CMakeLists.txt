add_subdirectory(core)
add_subdirectory(app)
add_subdirectory(ui)

qt_add_executable(giga-strata WIN32 main.cpp)
giga_strata_warnings(giga-strata)
target_link_libraries(giga-strata PRIVATE giga_app giga_appplugin giga_ui giga_uiplugin giga_core
                                          Qt6::Widgets Qt6::Quick Qt6::QuickControls2)
