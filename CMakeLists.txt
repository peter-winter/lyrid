cmake_minimum_required(VERSION 3.20)
project(lyrid LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_TESTS "Enable testing" ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

if(ENABLE_TESTS)
    include(CTest)
endif()

# Subdirectories
add_subdirectory(src/lyrid_parser)
add_subdirectory(src/lyrid_runtime)
add_subdirectory(src/lyrid_cli)
