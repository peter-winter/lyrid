# src/lyrid_runtime/CMakeLists.txt

include(FetchContent)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.23  # Latest release as of December 31, 2025
)

FetchContent_MakeAvailable(miniaudio)

add_library(miniaudio_lib INTERFACE)
target_include_directories(miniaudio_lib INTERFACE ${miniaudio_SOURCE_DIR})

add_compile_definitions(
        MA_NO_DECODING
        MA_NO_ENCODING
        MA_NO_GENERATION
        MA_NO_RESOURCE_MANAGER
        MA_NO_NODE_GRAPH
        MA_NO_THREADING
)

add_library(lyrid_runtime STATIC runtime.c)

target_link_libraries(lyrid_runtime
    PUBLIC
        lyrid_parser
        miniaudio_lib
)

target_include_directories(lyrid_runtime
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)


