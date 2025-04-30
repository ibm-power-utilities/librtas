// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems Vital Product Data (VPD)
// via /dev/papr-platform-dump or the legacy rtas() syscall.

#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <search.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "internal.h"
#include "librtas.h"
#include "papr-platform-dump.h"

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
int platform_dump_user(uint64_t dump_tag, uint64_t sequence, void *buffer,
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

static bool platform_dump_can_use_chardev(void)
{
	struct stat statbuf;

	if (stat("/dev/papr-platform-dump", &statbuf))
		return false;

	if (!S_ISCHR(statbuf.st_mode))
		return false;

	if (close(open("/dev/papr-platform-dump", O_RDONLY)))
		return false;

	return true;
}

#define DEVPATH "/dev/papr-platform-dump"

static int platform_dump_fd_new(uint64_t dump_tag)
{
	const int devfd = open(DEVPATH, O_WRONLY);
	int fd = -1;

	if (devfd < 0)
		return -1;

	fd = ioctl(devfd, PAPR_PLATFORM_DUMP_IOC_CREATE_HANDLE, &dump_tag);

	close(devfd);
	return fd;
}

int platform_dump_kernel(uint64_t dump_tag, uint64_t sequence, void *buffer,
		size_t length, uint64_t *seq_next, uint64_t *bytes_ret)
{
	int fd = (sequence == 0) ? platform_dump_fd_new(dump_tag)
				: (int)sequence;
	int rtas_status = 0;
	ssize_t size;

	/* Synthesize ibm,get-platfrom-dump "parameter error" */
	if (fd < 0)
		return -3;

	/*
	 * rtas_platform_dump() is called with buf = NULL and length = 0
	 * for "dump complete" RTAS call to invalidate dump.
	 * For kernel interface, read() will be continued until the
	 * return value = 0. Means kernel API will return this value only
	 * after the kernel RTAS call returned "dump complete" status
	 * and the hypervisor expects last RTAS call to invalidate dump.
	 * So issue the following ioctl API which invalidates the dump
	 * with the last RTAS call.
	 */
	if (buffer == NULL) {
		rtas_status = ioctl(fd, PAPR_PLATFORM_DUMP_IOC_INVALIDATE,
				&dump_tag);
		close(fd);
		return rtas_status;
	}

	/*
	 * Ensure we return a fd > 0 in seq_next.
	 */
	if (fd == 0) {
		int newfd = dup(fd);
		close(fd);
		fd = newfd;
	}

	size = read(fd, buffer, length);
	if (size < 0) {
		/* Synthesize ibm,get-platfrom-dump "hardware error" */
		close(fd);
		return -1;
	} else if (size > 0) {
		rtas_status = 1; /* More data available, call again */
	}

	if (seq_next)
		*seq_next = fd;
	if (bytes_ret)
		*bytes_ret = size;

	return rtas_status;
}

static int (*platform_dump_fn)(uint64_t dump_tag, uint64_t sequence,
				void *buffer, size_t length,
				uint64_t *seq_next, uint64_t *bytes_ret);

static void platform_dump_fn_setup(void)
{
	platform_dump_fn = platform_dump_can_use_chardev() ?
		platform_dump_kernel : platform_dump_user;
}

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
	static pthread_once_t platform_dump_fn_once = PTHREAD_ONCE_INIT;

	pthread_once(&platform_dump_fn_once, platform_dump_fn_setup);

	return platform_dump_fn(dump_tag, sequence, buffer, length, seq_next,
			bytes_ret);
}

