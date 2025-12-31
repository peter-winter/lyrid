#include "unity.h"
#include "lyrid/parser.h"
#include "lyrid/ast_printer.h"
#include <string.h>

void setUp(void)
{
    /* Optional: Called before each test */
}

void tearDown(void)
{
    /* Optional: Called after each test */
}

void test_scalar_assignment(void)
{
    const char *input = "int b=2\n";
    lyrid_program *prog = lyrid_parse_string(input);

    TEST_ASSERT_NOT_NULL(prog);
    TEST_ASSERT_TRUE(prog->is_valid);

    char *regenerated = lyrid_print_program(prog);
    TEST_ASSERT_NOT_NULL(regenerated);
    TEST_ASSERT_EQUAL_STRING(input, regenerated);

    free(regenerated);
    lyrid_ast_free(prog);
}

void test_array_assignment(void)
{
    const char *input = "float[]a=[4.6,-3.2]\n";
    lyrid_program *prog = lyrid_parse_string(input);

    TEST_ASSERT_NOT_NULL(prog);
    TEST_ASSERT_TRUE(prog->is_valid);

    char *regenerated = lyrid_print_program(prog);
    TEST_ASSERT_NOT_NULL(regenerated);
    TEST_ASSERT_EQUAL_STRING(input, regenerated);

    free(regenerated);
    lyrid_ast_free(prog);
}

void test_function_call_assignment(void)
{
    const char *input = "float x=foo(a,b)\n";
    lyrid_program *prog = lyrid_parse_string(input);

    TEST_ASSERT_NOT_NULL(prog);
    TEST_ASSERT_TRUE(prog->is_valid);

    char *regenerated = lyrid_print_program(prog);
    TEST_ASSERT_NOT_NULL(regenerated);
    TEST_ASSERT_EQUAL_STRING(input, regenerated);

    free(regenerated);
    lyrid_ast_free(prog);
}

void test_multi_statement_program(void)
{
    const char *input = "int b=2\nfloat[]a=[4.6,-3.2]\nfloat x=foo(a,b)\n";
    lyrid_program *prog = lyrid_parse_string(input);

    TEST_ASSERT_NOT_NULL(prog);
    TEST_ASSERT_TRUE(prog->is_valid);

    char *regenerated = lyrid_print_program(prog);
    TEST_ASSERT_NOT_NULL(regenerated);
    TEST_ASSERT_EQUAL_STRING(input, regenerated);

    free(regenerated);
    lyrid_ast_free(prog);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_scalar_assignment);
    RUN_TEST(test_array_assignment);
    RUN_TEST(test_function_call_assignment);
    RUN_TEST(test_multi_statement_program);
    return UNITY_END();
}
