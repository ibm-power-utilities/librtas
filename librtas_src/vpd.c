// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems Vital Product Data (VPD)
// via the rtas() syscall.

#include <endian.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscall.h>

#include "internal.h"
#include "librtas.h"

/**
 * rtas_get_vpd
 * @brief Interface to the ibm,get-vpd rtas call
 *
 * @param loc_code location code
 * @param workarea additional args to rtas call
 * @param size
 * @param sequence
 * @param seq_next
 * @param bytes_ret
 * @return 0 on success, !0 otherwise
 */
int rtas_get_vpd(char *loc_code, char *workarea, size_t size,
		 unsigned int sequence, unsigned int *seq_next,
		 unsigned int *bytes_ret)
{
	uint32_t kernbuf_pa;
	uint32_t loc_pa = 0;
	uint32_t rmo_pa = 0;
	uint64_t elapsed = 0;
	void *kernbuf;
	void *rmobuf;
	void *locbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(size + WORK_AREA_SIZE, &rmobuf, &rmo_pa);
	if (rc)
		return rc;

	kernbuf = rmobuf + WORK_AREA_SIZE;
	kernbuf_pa = rmo_pa + WORK_AREA_SIZE;
	locbuf = rmobuf;
	loc_pa = rmo_pa;

	/* If user didn't set loc_code, copy a NULL string */
	strncpy(locbuf, loc_code ? loc_code : "", WORK_AREA_SIZE);

	*seq_next = htobe32(sequence);
	do {
		sequence = *seq_next;
		rc = rtas_call_no_delay("ibm,get-vpd", 4, 3, htobe32(loc_pa),
				  htobe32(kernbuf_pa), htobe32(size),
				  sequence, &status, seq_next,
				  bytes_ret);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, size);

	(void) rtas_free_rmo_buffer(rmobuf, rmo_pa, size + WORK_AREA_SIZE);

	*seq_next = be32toh(*seq_next);
	*bytes_ret = be32toh(*bytes_ret);

	dbg("(%s, 0x%p, %zu, %u) = %d, %u, %u\n", loc_code ? loc_code : "NULL",
	    workarea, size, sequence, status, *seq_next, *bytes_ret);
	return rc ? rc : status;
}
