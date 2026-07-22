cmake_minimum_required(VERSION 3.20)

project(MatrixFillingGameSearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mfg_core
    src/cnf.cpp
    src/model.cpp
    src/search.cpp
)
target_include_directories(mfg_core PUBLIC include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mfg_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(mfg_search src/main.cpp)
target_link_libraries(mfg_search PRIVATE mfg_core)

enable_testing()
add_executable(mfg_tests tests/test_main.cpp)
target_link_libraries(mfg_tests PRIVATE mfg_core)
add_test(NAME mfg_tests COMMAND mfg_tests)
