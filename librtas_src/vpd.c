// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems Vital Product Data (VPD)
// via /dev/papr-vpd or the legacy rtas() syscall.

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <search.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "internal.h"
#include "librtas.h"
#include "papr-vpd.h"

static int
get_vpd_syscall_fallback(const char *loc_code, char *workarea, size_t size,
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

static bool get_vpd_can_use_chardev(void)
{
	struct stat statbuf;

	if (stat("/dev/papr-vpd", &statbuf))
		return false;

	if (!S_ISCHR(statbuf.st_mode))
		return false;

	if (close(open("/dev/papr-vpd", O_RDONLY)))
		return false;

	return true;
}

#define DEVPATH "/dev/papr-vpd"

static int vpd_fd_new(const char *loc_code)
{
	const int devfd = open(DEVPATH, O_WRONLY);
	struct papr_location_code lc = {};
	int fd = -1;

	if (devfd < 0)
		return -1;

	if (!loc_code)
		loc_code = "";

	if (sizeof(lc.str) < strlen(loc_code))
		goto close_devfd;

	strncpy(lc.str, loc_code, sizeof(lc.str));
	fd = ioctl(devfd, PAPR_VPD_IOC_CREATE_HANDLE, &lc);
close_devfd:
	close(devfd);
	return fd;
}

static int
get_vpd_chardev(const char *loc_code, char *workarea, size_t size,
		unsigned int sequence, unsigned int *seq_next,
		unsigned int *bytes_ret)
{
	int fd = (sequence == 1) ? vpd_fd_new(loc_code) : (int)sequence;

	/*
	 * Ensure we return a fd > 0 in seq_next.
	 */
	if (fd == 1) {
		int newfd = dup(fd);
		close(fd);
		fd = newfd;
	}

	if (fd < 0)
		return -3; /* Synthesize ibm,get-vpd "parameter error" */

	int rtas_status = 0;
	ssize_t res = read(fd, workarea, size);
	if (res < 0) {
		rtas_status = -1; /* Synthesize ibm,get-vpd "hardware error" */
		close(fd);
	} else if (res == 0 || res < (ssize_t)size) {
		rtas_status = 0; /* Done with sequence, no more data */
		close(fd);
		if (seq_next)
			*seq_next = 1;
		if (bytes_ret)
			*bytes_ret = res;
	} else {
		rtas_status = 1; /* More data available, call again */
		if (seq_next)
			*seq_next = fd;
		if (bytes_ret)
			*bytes_ret = res;
	}

	return rtas_status;
}

static int (*get_vpd_fn)(const char *loc_code,
			 char *workarea,
			 size_t size,
			 unsigned int sequence,
			 unsigned int *seq_next,
			 unsigned int *bytes_ret);

static void get_vpd_fn_setup(void)
{
	get_vpd_fn = get_vpd_can_use_chardev() ?
		get_vpd_chardev : get_vpd_syscall_fallback;
}

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
	static pthread_once_t get_vpd_fn_once = PTHREAD_ONCE_INIT;

	pthread_once(&get_vpd_fn_once, get_vpd_fn_setup);

	return get_vpd_fn(loc_code, workarea, size, sequence,
			  seq_next, bytes_ret);
}
