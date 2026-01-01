cmake_minimum_required(VERSION 3.12)
project(lyrid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
)

FetchContent_MakeAvailable(Catch2)

# Enable CTest integration
enable_testing()

# Add the Catch2 extras directory to the module path for test discovery
list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

# Include the Catch2 CMake module
include(Catch)

add_library(lyrid_parser STATIC
    src/parser.cpp
    src/semantic_analyzer.cpp
)

target_include_directories(lyrid_parser
    PUBLIC include
)

add_executable(lyrid_parser_tests
    tests/test_parser.cpp
)

target_include_directories(lyrid_parser_tests
    PRIVATE include
)

target_link_libraries(lyrid_parser_tests
    PRIVATE
        lyrid_parser
        Catch2::Catch2WithMain
)

# Automatically discover and register Catch2 tests with CTest
catch_discover_tests(lyrid_parser_tests)
