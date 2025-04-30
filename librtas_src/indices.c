// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems indices (indicator and sensor)
// data via /dev/papr-indices or the legacy rtas() syscalls.

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sys/syscall.h>

#include "internal.h"
#include "librtas.h"

/**
 * rtas_get_dynamic_sensor
 * @brief Interface to ibm,get-dynamic-sensor-state rtas call
 *
 * On success the variable referenced by the state parameter will contain
 * the state of the sensor
 *
 * @param sensor sensor to retrieve
 * @param loc_code location code of the sensor
 * @param state reference to state variable
 * @return 0 on success, !0 otherwise
 */
int rtas_get_dynamic_sensor(int sensor, void *loc_code, int *state)
{
	uint32_t loc_pa = 0;
	void *locbuf;
	uint32_t size;
	__be32 be_state;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	size = be32toh(*(uint32_t *)loc_code) + sizeof(uint32_t);

	rc = rtas_get_rmo_buffer(size, &locbuf, &loc_pa);
	if (rc)
		return rc;

	memcpy(locbuf, loc_code, size);

	rc = rtas_call("ibm,get-dynamic-sensor-state", 2, 2,
		       htobe32(sensor), htobe32(loc_pa), &status, &be_state);

	(void) rtas_free_rmo_buffer(locbuf, loc_pa, size);

	*state = be32toh(be_state);

	dbg("(%d, %s, %p) = %d, %d\n", sensor, (char *)loc_code, state,
	    rc ? rc : status, *state);
	return rc ? rc : status;
}

/**
 * rtas_set_dynamic_indicator
 * @brief Interface to the ibm,set-dynamic-indicator rtas call
 *
 * @param indicator indicator to set
 * @param new_value value to set the indicator to
 * @param loc_code
 * @return 0 on success, !0 otherwise
 */
int rtas_set_dynamic_indicator(int indicator, int new_value, void *loc_code)
{
	uint32_t loc_pa = 0;
	void *locbuf;
	uint32_t size;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	size = be32toh(*(uint32_t *)loc_code) + sizeof(uint32_t);

	rc = rtas_get_rmo_buffer(size, &locbuf, &loc_pa);
	if (rc)
		return rc;

	memcpy(locbuf, loc_code, size);

	rc = rtas_call("ibm,set-dynamic-indicator", 3, 1, htobe32(indicator),
		       htobe32(new_value), htobe32(loc_pa), &status);

	(void) rtas_free_rmo_buffer(locbuf, loc_pa, size);

	dbg("(%d, %d, %s) = %d\n", indicator, new_value, (char *)loc_code,
	    rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_get_indices
 * @brief Interface to the ibm,get-indices rtas call
 *
 * @param is_sensor is this index a sensor?
 * @param type
 * @param workarea additional args to the rtas call
 * @param size
 * @param start
 * @param next
 * @return 0 on success, !0 otherwise
 */
int rtas_get_indices(int is_sensor, int type, char *workarea, size_t size,
		     int start, int *next)
{
	uint32_t kernbuf_pa;
	__be32 be_next;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(size, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	rc = rtas_call("ibm,get-indices", 5, 2, htobe32(is_sensor),
		       htobe32(type), htobe32(kernbuf_pa), htobe32(size),
		       htobe32(start), &status, &be_next);

	if (rc == 0)
		memcpy(workarea, kernbuf, size);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, size);

	*next = be32toh(be_next);

	dbg("(%d, %d, %p, %zu, %d, %p) = %d, %d\n", is_sensor, type, workarea,
	     size, start, next, rc ? rc : status, *next);
	return rc ? rc : status;
}
