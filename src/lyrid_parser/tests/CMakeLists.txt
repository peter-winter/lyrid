# src/lyrid_parser/tests/CMakeLists.txt

include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.1  # Latest release as of December 31, 2025
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(unity)

add_executable(parser_tests parser_tests.c)

target_link_libraries(parser_tests
    PRIVATE
        lyrid_parser
        unity
)

target_include_directories(parser_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

add_test(NAME ParserTests COMMAND parser_tests)
