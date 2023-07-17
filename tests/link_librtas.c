#include <librtas.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

static void call_rtas_set_debug(void **state)
{
	(void)rtas_set_debug(0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(call_rtas_set_debug),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
