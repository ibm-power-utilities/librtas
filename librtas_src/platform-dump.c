// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems Vital Product Data (VPD)
// via /dev/papr-platform-dump or the legacy rtas() syscall.

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

#include "internal.h"
#include "librtas.h"


/**
 * rtas_platform_dump
 * Interface to the ibm,platform-dump rtas call
 *
 * @param dump_tag
 * @param sequence
 * @param buffer buffer to write dump to
 * @param length buffer length
 * @param next_seq
 * @param bytes_ret
 * @return 0 on success, !0 othwerwise
 */
int rtas_platform_dump(uint64_t dump_tag, uint64_t sequence, void *buffer,
		       size_t length, uint64_t *seq_next, uint64_t *bytes_ret)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa = 0;
	uint32_t next_hi, next_lo;
	uint32_t bytes_hi, bytes_lo;
	uint32_t dump_tag_hi, dump_tag_lo;
	void *kernbuf = NULL;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	if (buffer) {
		rc = rtas_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
		if (rc)
			return rc;
	}

	/* Converting a 64bit host value to 32bit BE, _hi and _lo
	 * pair is tricky: we should convert the _hi and _lo 32bits
	 * of the 64bit host value.
	 */
	dump_tag_hi = htobe32(BITS32_HI(dump_tag));
	dump_tag_lo = htobe32(BITS32_LO(dump_tag));

	next_hi = htobe32(BITS32_HI(sequence));
	next_lo = htobe32(BITS32_LO(sequence));

	do {
		rc = rtas_call_no_delay("ibm,platform-dump", 6, 5, dump_tag_hi,
					dump_tag_lo, next_hi, next_lo,
					htobe32(kernbuf_pa), htobe32(length),
					&status, &next_hi, &next_lo,
					&bytes_hi, &bytes_lo);
		if (rc < 0)
			break;

		sequence = BITS64(be32toh(next_hi), be32toh(next_lo));
		dbg("%s: seq_next = 0x%" PRIx64 "\n", __FUNCTION__, sequence);

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (buffer && (rc == 0))
		memcpy(buffer, kernbuf, length);

	if (kernbuf)
		(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	*seq_next = sequence;
	bytes_hi = be32toh(bytes_hi);
	bytes_lo = be32toh(bytes_lo);
	*bytes_ret = BITS64(bytes_hi, bytes_lo);

	dbg("(0x%"PRIx64", 0x%"PRIx64", %p, %zu, %p, %p) = %d, 0x%"PRIx64", 0x%"PRIx64"\n",
	     dump_tag, sequence, buffer, length, seq_next, bytes_ret,
	     rc ? rc : status, *seq_next, *bytes_ret);
	return rc ? rc : status;
}

