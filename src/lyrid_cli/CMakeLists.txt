# src/lyrid_cli/CMakeLists.txt

add_executable(lyrid_cli main.c)

target_link_libraries(lyrid_cli
    PRIVATE
        lyrid_runtime
)

target_include_directories(lyrid_cli
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
