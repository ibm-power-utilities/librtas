#include <dlfcn.h>
#include <errno.h>
#include <librtas.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

static void test_dlsym(void **state, const char *symbol)
{
	/*
	 * Clear any prior errors.
	 */
	(void)dlerror();

	void *handle = *state;
	const void *addr = dlsym(handle, symbol);
	/*
	 * dlsym() can return NULL for a successful lookup in some
	 * cases (see the manual), but for this test we can safely
	 * assume that a NULL result is a failure.
	 */
	assert_non_null(addr);
}

#define test_fn_name(str_) test_dlsym_ ## str_

#define define_test_fn(str_) \
	static void test_fn_name(str_)(void **state) {	 \
		test_dlsym(state, # str_);		 \
	}

define_test_fn(rtas_activate_firmware)
define_test_fn(rtas_cfg_connector)
define_test_fn(rtas_delay_timeout)
define_test_fn(rtas_display_char)
define_test_fn(rtas_display_msg)
define_test_fn(rtas_errinjct)
define_test_fn(rtas_errinjct_close)
define_test_fn(rtas_errinjct_open)
define_test_fn(rtas_free_rmo_buffer)
define_test_fn(rtas_get_config_addr_info2)
define_test_fn(rtas_get_dynamic_sensor)
define_test_fn(rtas_get_indices)
define_test_fn(rtas_get_power_level)
define_test_fn(rtas_get_rmo_buffer)
define_test_fn(rtas_get_sensor)
define_test_fn(rtas_get_sysparm)
define_test_fn(rtas_get_time)
define_test_fn(rtas_get_vpd)
define_test_fn(rtas_lpar_perftools)
define_test_fn(rtas_platform_dump)
define_test_fn(rtas_read_slot_reset)
define_test_fn(rtas_scan_log_dump)
define_test_fn(rtas_set_debug)
define_test_fn(rtas_set_dynamic_indicator)
define_test_fn(rtas_set_eeh_option)
define_test_fn(rtas_set_indicator)
define_test_fn(rtas_set_power_level)
define_test_fn(rtas_set_poweron_time)
define_test_fn(rtas_set_sysparm)
define_test_fn(rtas_set_time)
define_test_fn(rtas_suspend_me)
define_test_fn(rtas_update_nodes)
define_test_fn(rtas_update_properties)
define_test_fn(rtas_physical_attestation)

static int setup(void **state)
{
	/*
	 * Clear any prior errors.
	 */
	(void)dlerror();

	const char librtas_path[] = ".libs/librtas.so";
	void *handle = dlopen(librtas_path, RTLD_NOW);
	if (!handle) {
		cm_print_error("failed to dlopen %s: %s\n", librtas_path, dlerror());
		return 1;
	}

	*state = handle;
	return 0;
}

static int teardown(void **state)
{
	void *handle = *state;

	/*
	 * Do not proceed if setup failed.
	 */
	if (!handle)
		return 0;
	/*
	 * Clear any prior errors.
	 */
	(void)dlerror();

	int err = dlclose(handle);
	if (err != 0) {
		cm_print_error("dlclose failure: %s\n", dlerror());
		return 1;
	}

	return 0;
}

int main(void) {
#define TT(str_) cmocka_unit_test(str_)
#define T(str_) TT(test_fn_name(str_))
	const struct CMUnitTest tests[] = {
		T(rtas_activate_firmware),
		T(rtas_cfg_connector),
		T(rtas_delay_timeout),
		T(rtas_display_char),
		T(rtas_display_msg),
		T(rtas_errinjct),
		T(rtas_errinjct_close),
		T(rtas_errinjct_open),
		T(rtas_free_rmo_buffer),
		T(rtas_get_config_addr_info2),
		T(rtas_get_dynamic_sensor),
		T(rtas_get_indices),
		T(rtas_get_power_level),
		T(rtas_get_rmo_buffer),
		T(rtas_get_sensor),
		T(rtas_get_sysparm),
		T(rtas_get_time),
		T(rtas_get_vpd),
		T(rtas_lpar_perftools),
		T(rtas_platform_dump),
		T(rtas_read_slot_reset),
		T(rtas_scan_log_dump),
		T(rtas_set_debug),
		T(rtas_set_dynamic_indicator),
		T(rtas_set_eeh_option),
		T(rtas_set_indicator),
		T(rtas_set_power_level),
		T(rtas_set_poweron_time),
		T(rtas_set_sysparm),
		T(rtas_set_time),
		T(rtas_suspend_me),
		T(rtas_update_nodes),
		T(rtas_update_properties),
		T(rtas_physical_attestation),
	};

	return cmocka_run_group_tests(tests, setup, teardown);
}
