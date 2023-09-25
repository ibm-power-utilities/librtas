#include <stdint.h>
#include <string.h>
#include "internal.h"
#include "librtas.h"

/**
 * rtas_get_sysparm
 * @brief Interface to the ibm,get-system-parameter rtas call
 *
 * On successful completion the data parameter will contain the system
 * parameter results
 *
 * @param parameter system parameter to retrieve
 * @param length data buffer length
 * @param data reference to buffer to return parameter in
 * @return 0 on success, !0 otherwise
 */
int rtas_get_sysparm(unsigned int parameter, unsigned int length, char *data)
{
	uint32_t kernbuf_pa;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	rc = rtas_call("ibm,get-system-parameter", 3, 1, htobe32(parameter),
		       htobe32(kernbuf_pa), htobe32(length), &status);

	if (rc == 0)
		memcpy(data, kernbuf, length);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	dbg("(%u, %u, %p) = %d\n", parameter, length, data, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_set_sysparm
 * @brief Interface to the ibm,set-system-parameter rtas call
 *
 * @param parameter
 * @param data
 * @return 0 on success, !0 otherwise
 */
int rtas_set_sysparm(unsigned int parameter, char *data)
{
	uint32_t kernbuf_pa;
	void *kernbuf;
	int rc, status;
	uint16_t size;

	rc = sanity_check();
	if (rc)
		return rc;
	/*
	 * We have to copy the contents of @data to a RMO buffer. The
	 * caller has encoded the data length in the first two bytes
	 * of @data in MSB order, and we can't assume any
	 * alignment. So interpret @data as:
	 *
	 * struct {
	 *     unsigned char len_msb;
	 *     unsigned char len_lsb;
	 *     char [(len_msb << 8) + len_lsb];
	 * }
	 */
	size = 2 + (((unsigned char)data[0] << 8) | (unsigned char)data[1]);
	rc = rtas_get_rmo_buffer(size, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, data, size);

	rc = rtas_call("ibm,set-system-parameter", 2, 1, htobe32(parameter),
		       htobe32(kernbuf_pa), &status);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, size);

	dbg("(%u, %p) = %d\n", parameter, data, rc ? rc : status);
	return rc ? rc : status;
}
