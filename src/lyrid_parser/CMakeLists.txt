# src/lyrid_parser/CMakeLists.txt

BISON_TARGET(Parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.h)
FLEX_TARGET(Lexer lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_library(lyrid_parser STATIC
    ast.c
    ast_builder.c
    ast_printer.c
    parser.c
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)

target_include_directories(lyrid_parser
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
