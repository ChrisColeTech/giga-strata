cmake_minimum_required(VERSION 3.25)
# MSVC runtime checks as a target property, so the knapsack can drop them (src/core/CMakeLists.txt).
if(POLICY CMP0184)
    cmake_policy(SET CMP0184 NEW)
endif()
project(giga-strata VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# exe, test exes and the windeployqt output share one folder (docs/03 section 5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Concurrent Svg Test)
qt_standard_project_setup(REQUIRES 6.8)
# Every QML module lands under one import root at its URI path, so qmllint over src/ui finds
# GigaStrata.App (R9: qmllint must be clean).
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

# R32: warnings are errors in project code; Qt headers (angle-bracket includes) stay silent
function(giga_strata_warnings target)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8 /external:anglebrackets /external:W0)
    target_compile_definitions(${target} PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
                                                 NOMINMAX WIN32_LEAN_AND_MEAN)
endfunction()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
