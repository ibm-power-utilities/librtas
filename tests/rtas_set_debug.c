#include <librtas.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

static void test_rtas_set_debug(void **state, int dbgval)
{
	errno = 0;

	int ret = rtas_set_debug(dbgval);

	/*
	 * Should always return 0 without errors.
	 */
	assert_int_equal(ret, 0);
	assert_int_equal(errno, 0);
}

#define test_fn_name(val) test_rtas_set_debug_ ## val

#define define_test_fn(val) \
	static void test_fn_name(val)(void **state)	\
	{						\
		test_rtas_set_debug(state, val);	\
	}

define_test_fn(0)
define_test_fn(1)
define_test_fn(2)
define_test_fn(3)
define_test_fn(4)
define_test_fn(5)
define_test_fn(6)
define_test_fn(7)
define_test_fn(8)
define_test_fn(9)
define_test_fn(10)

int main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_fn_name(0)),
		cmocka_unit_test(test_fn_name(1)),
		cmocka_unit_test(test_fn_name(2)),
		cmocka_unit_test(test_fn_name(3)),
		cmocka_unit_test(test_fn_name(4)),
		cmocka_unit_test(test_fn_name(5)),
		cmocka_unit_test(test_fn_name(6)),
		cmocka_unit_test(test_fn_name(7)),
		cmocka_unit_test(test_fn_name(8)),
		cmocka_unit_test(test_fn_name(9)),
		cmocka_unit_test(test_fn_name(10)),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
