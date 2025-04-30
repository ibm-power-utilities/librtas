// SPDX-License-Identifier: LGPL-2.1-or-later
  
// Support for accessing ibm,physical-attestation data
// via /dev/papr-phy-attestation or the legacy rtas() syscall.

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include "internal.h"
#include "librtas.h"

/**
 * rtas_physical_attestation
 * @brief Interface for ibm,physical-attestation rtas call.
 *
 * @param workarea input/output work area for rtas call
 * @param seq_num sequence number of the rtas call
 * @param next_seq_num next sequence number
 * @param work_area_bytes size of work area
 * @return 0 on success, !0 on failure
 */
int rtas_physical_attestation(char *workarea, int seq_num, int *next_seq_num,
			      int *work_area_bytes)
{
	uint32_t workarea_pa;
	uint64_t elapsed = 0;
	void *kernbuf;
	int kbuf_sz = WORK_AREA_SIZE;
	int rc, status;
	int resp_bytes = *work_area_bytes;

	rc = sanity_check();
	if (rc)
		return rc;

	/* Caller provided more data than FW can handle */
	if (*work_area_bytes == 0 ||
	    *work_area_bytes > kbuf_sz)
		return RTAS_IO_ASSERT;

	rc = rtas_get_rmo_buffer(kbuf_sz, &kernbuf, &workarea_pa);
	if (rc)
		return rc;
	memcpy(kernbuf, workarea, *work_area_bytes);

	do {
		rc = rtas_call("ibm,physical-attestation", 3, 3,
			       htobe32(workarea_pa), htobe32(kbuf_sz),
			       htobe32(seq_num),
			       &status, next_seq_num, &resp_bytes);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*next_seq_num = be32toh(*next_seq_num);

	/* FW returned more data than we can handle */
	if (be32toh(resp_bytes) > (unsigned int)*work_area_bytes) {
		(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, kbuf_sz);
		return RTAS_IO_ASSERT;
	}

	*work_area_bytes = be32toh(resp_bytes);

	if (rc == 0)
		memcpy(workarea, kernbuf, *work_area_bytes);

	(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, kbuf_sz);

	return rc ? rc : status;
}

