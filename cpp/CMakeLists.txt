cmake_minimum_required(VERSION 3.21)

project(MatrixFillingGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
include(CTest)
include(GNUInstallDirs)
find_package(Threads REQUIRED)
option(MFG_BUILD_GUI "Build the Qt 6 desktop application" ON)

add_library(mfg_core
    src/model.cpp
    src/cnf.cpp
    src/search.cpp
    src/document.cpp
    src/analysis.cpp
    src/automaton.cpp
    src/polynomial.cpp
)
target_include_directories(mfg_core PUBLIC include)
target_link_libraries(mfg_core PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(mfg_core PRIVATE /W4 /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mfg_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(mfg_search src/cli/search_main.cpp)
target_link_libraries(mfg_search PRIVATE mfg_core)
add_executable(mfg_analyze src/cli/analyze_main.cpp)
target_link_libraries(mfg_analyze PRIVATE mfg_core)
add_executable(mfg_tools src/cli/tools_main.cpp)
target_link_libraries(mfg_tools PRIVATE mfg_core)
install(TARGETS mfg_search mfg_analyze mfg_tools RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
    foreach(test IN ITEMS main document analysis utilities)
        add_executable(mfg_${test}_tests tests/test_${test}.cpp)
        target_link_libraries(mfg_${test}_tests PRIVATE mfg_core)
        add_test(NAME mfg_${test}_tests COMMAND mfg_${test}_tests)
    endforeach()
    add_test(NAME cli_diagonal COMMAND mfg_analyze ${CMAKE_CURRENT_SOURCE_DIR}/examples/diagonal.mfg --no-sampling)
    set_tests_properties(cli_diagonal PROPERTIES PASS_REGULAR_EXPRESSION "Valid completions: 3")
    add_test(NAME cli_nonadjacent COMMAND mfg_analyze ${CMAKE_CURRENT_SOURCE_DIR}/examples/nonadjacent.txt --raw --no-sampling)
    set_tests_properties(cli_nonadjacent PROPERTIES PASS_REGULAR_EXPRESSION "Valid completions: 0")
    add_test(NAME search_baseline COMMAND mfg_search --max-k 3 --no-projections)
endif()

if(MFG_BUILD_GUI)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
    add_library(mfg_desktop
        src/gui/main_window.cpp src/gui/main_window.hpp
        src/gui/automaton_dialog.cpp src/gui/automaton_dialog.hpp
    )
    set_target_properties(mfg_desktop PROPERTIES AUTOMOC ON)
    target_include_directories(mfg_desktop PUBLIC src/gui)
    target_link_libraries(mfg_desktop PUBLIC mfg_core Qt6::Widgets)
    if(MSVC)
        target_compile_options(mfg_desktop PRIVATE /utf-8)
    endif()
    qt_add_executable(mfg_gui src/gui/main.cpp)
    target_link_libraries(mfg_gui PRIVATE mfg_desktop)
    set_target_properties(mfg_gui PROPERTIES
        OUTPUT_NAME MatrixFillingGame
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER org.matrixfillinggame.desktop
        MACOSX_BUNDLE_BUNDLE_NAME MatrixFillingGame
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    install(TARGETS mfg_gui BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    qt_generate_deploy_app_script(TARGET mfg_gui FILENAME_VARIABLE qt_deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT ${qt_deploy_script})
    if(BUILD_TESTING)
        find_package(Qt6 REQUIRED COMPONENTS Test)
        add_executable(mfg_gui_tests tests/test_gui.cpp)
        set_target_properties(mfg_gui_tests PROPERTIES AUTOMOC ON)
        target_link_libraries(mfg_gui_tests PRIVATE mfg_desktop Qt6::Test)
        add_test(NAME mfg_gui_tests COMMAND mfg_gui_tests)
        set_tests_properties(mfg_gui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 90)
        add_executable(mfg_graph_gui_tests tests/test_graph_gui.cpp)
        set_target_properties(mfg_graph_gui_tests PROPERTIES AUTOMOC ON)
        target_link_libraries(mfg_graph_gui_tests PRIVATE mfg_desktop Qt6::Test)
        add_test(NAME mfg_graph_gui_tests COMMAND mfg_graph_gui_tests)
        set_tests_properties(mfg_graph_gui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
    endif()
endif()
