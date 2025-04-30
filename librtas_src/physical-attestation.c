// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing ibm,physical-attestation data
// via /dev/papr-phy-attestation or the legacy rtas() syscall.

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include "internal.h"
#include "librtas.h"
#include "papr-physical-attestation.h"

/**
 * ibm,physical-attestation RTAS call from user space
 *
 * @param workarea input/output work area for rtas call
 * @param seq_num sequence number of the rtas call
 * @param next_seq_num next sequence number
 * @param work_area_bytes size of work area
 * @return 0 on success, !0 on failure
 */
int phy_attestation_user(char *workarea, int seq_num, int *next_seq_num,
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

static bool phy_attest_can_use_chardev(void)
{
	struct stat statbuf;

	if (stat("/dev/papr-physical-attestation", &statbuf))
		return false;

	if (!S_ISCHR(statbuf.st_mode))
		return false;

	if (close(open("/dev/papr-physical-attestation", O_RDONLY)))
		return false;

	return true;
}

#define DEVPATH "/dev/papr-physical-attestation"

static int phy_attest_fd_new(const char *attest_cmd, unsigned int size)
{
	const int devfd = open(DEVPATH, O_WRONLY);
	struct papr_phy_attest_io_block cmd = {};
	int fd = -1;

	if (devfd < 0)
		return -1;

	/*
	 * Size of each command struct has to be the  buffer size
	 * (WORK_AREA_SIZE - 4K) passed by the user.
	 */
	if (size != sizeof(struct papr_phy_attest_io_block)) {
		fd = RTAS_IO_ASSERT;
		goto close_devfd;
	}

	memcpy(&cmd, attest_cmd, sizeof(struct papr_phy_attest_io_block));
	fd = ioctl(devfd, PAPR_PHY_ATTEST_IOC_HANDLE, &cmd);

close_devfd:
	close(devfd);
	return fd;
}

static int
phy_attestation_kernel(char *workarea, int seq_num, int *next_seq_num,
			int *work_area_bytes)
{
	int size = *work_area_bytes;
	int fd = (seq_num == 1) ? phy_attest_fd_new(workarea, size)
		: (int)seq_num;

	/*
	 * Ensure we return a fd > 0 in next_seq_num.
	 */
	if (fd == 1) {
		int newfd = dup(fd);
		close(fd);
		fd = newfd;
	}

	if (fd < 0)
		return -3; /* Synthesize ibm,get-vpd "parameter error" */

	/* Caller provided more data than FW can handle */
	if (size == 0 || size > WORK_AREA_SIZE) {
		close(fd);
                return RTAS_IO_ASSERT;
	}


	int rtas_status = 0;
	ssize_t res = read(fd, workarea, *work_area_bytes);
	if (res < 0) {
		rtas_status = -1; /* Synthesize ibm,get-vpd "hardware error" */
		close(fd);
	} else if (res == 0 || res < (ssize_t)size) {
		rtas_status = 0; /* Done with sequence, no more data */
		close(fd);
		if (next_seq_num)
			*next_seq_num = 1;
		if (work_area_bytes)
			*work_area_bytes = res;
	} else {
		rtas_status = 1; /* More data available, call again */
		if (next_seq_num)
			*next_seq_num = fd;
		if (work_area_bytes)
			*work_area_bytes = res;
	}

	return rtas_status;
}

static int (*phy_attestation_fn)(char *workarea, int seq_num,
			int *next_seq_num, int *work_area_bytes);

static void phy_attestation_fn_setup(void)
{
	phy_attestation_fn = phy_attest_can_use_chardev() ?
			phy_attestation_kernel : phy_attestation_user;
}

/**
 * rtas_physical_attestation
 * @brief Interface for ibm,physical-attestation rtas call.
 *
 * @param workarea input/output work area for rtas call
 * @param seq_num sequence number of the rtas call
 * @param next_seq_num next sequence number
 * @param work_area_bytes size of input/output work area
 * @return 0 on success, !0 on failure
 */
int rtas_physical_attestation(char *workarea, int seq_num, int *next_seq_num,
				int *work_area_bytes)
{
	static pthread_once_t phy_attestation_fn_once = PTHREAD_ONCE_INIT;

	pthread_once(&phy_attestation_fn_once, phy_attestation_fn_setup);

	return phy_attestation_fn(workarea, seq_num, next_seq_num,
			work_area_bytes);
}
