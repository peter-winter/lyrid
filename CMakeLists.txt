cmake_minimum_required(VERSION 3.15)
project(lyrid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.23
    GIT_SHALLOW    TRUE
)

set(MINIAUDIO_NO_LIBVORBIS ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBOPUS   ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_WAV       ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_FLAC      ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_MP3       ON CACHE BOOL "" FORCE)   
set(MINIAUDIO_NO_EXTRA_NODES ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBVORBIS  ON CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBOPUS    ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(miniaudio)

add_executable(lyrid 
    src/main.cpp
    src/device.cpp
)

target_include_directories(lyrid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(lyrid PRIVATE miniaudio)   


