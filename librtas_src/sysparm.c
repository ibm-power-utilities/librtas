#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "internal.h"
#include "librtas.h"
#include "papr-sysparm.h"

static const char sysparm_devpath[] = "/dev/papr-sysparm";

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
static int
get_sysparm_syscall_fallback(unsigned int parameter, unsigned int length, char *data)
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
static int
set_sysparm_syscall_fallback(unsigned int parameter, char *data)
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

static bool sysparm_can_use_chardev(void)
{
	struct stat statbuf;

	if (stat(sysparm_devpath, &statbuf))
		return false;

	if (!S_ISCHR(statbuf.st_mode))
		return false;

	if (close(open(sysparm_devpath, O_RDONLY)))
		return false;

	return true;
}

/*
 * Only to be used when converting an actual error from a syscall.
 */
static int chardev_backconvert_errno(int saved_errno)
{
	const struct {
		int linux_errno;
		int rtas_status;
	} map[] = {
#define errno_to_status(e, s) { .linux_errno = (e), .rtas_status = (s), }
		errno_to_status(EINVAL,     -9999),
		errno_to_status(EPERM,      -9002),
		errno_to_status(EOPNOTSUPP,    -3),
		errno_to_status(EIO,           -1),
		errno_to_status(EFAULT,        -1),
#undef errno_to_status
	};

	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
		if (map[i].linux_errno == saved_errno)
			return map[i].rtas_status;
	return -1;
}

static int get_sysparm_chardev(unsigned int parameter, unsigned int length, char *data)
{
	const int fd = open(sysparm_devpath, O_RDWR);
	struct papr_sysparm_io_block buf = {
		.parameter = parameter,
	};

	if (fd < 0) {
		/*
		 * Really shouldn't get here without misconfiguration,
		 * e.g. removal of /dev/papr-sysparm. Synthesize a
		 * hardware/platform error.
		 */
		return -1;
	}

	/*
	 * It might make sense to have special handling for parameter
	 * 60 (OS Service Entitlement Status), which takes input data,
	 * but librtas has never handled that one correctly. So ignore
	 * it for now and don't copy incoming data into the block we
	 * pass to PAPR_SYSPARM_IOC_GET.
	 */

	const int res = ioctl(fd, PAPR_SYSPARM_IOC_GET, &buf);
	const int saved_errno = errno;
	(void)close(fd);

	if (res != 0)
		return chardev_backconvert_errno(saved_errno);

	const uint16_t result_size_msb = htobe16(buf.length);
	memcpy(data, &result_size_msb, sizeof(result_size_msb));
	length -= sizeof(result_size_msb);
	data += sizeof(result_size_msb);

	/*
	 * Copy no more than min(@length, sizeof(buf.data)).
	 */
	const size_t copy_size = sizeof(buf.data) < length ?
		sizeof(buf.data) : length;
	memcpy(data, buf.data, copy_size);
	return 0;
}

static int set_sysparm_chardev(unsigned int parameter, char *data)
{
	const int fd = open(sysparm_devpath, O_RDWR);
	struct papr_sysparm_io_block buf = {
		.parameter = parameter,
		.length = (((unsigned char)data[0] << 8) | (unsigned char)data[1]),
	};

	memcpy(buf.data, data + 2, buf.length);

	const int res = ioctl(fd, PAPR_SYSPARM_IOC_SET, &buf);
	const int saved_errno = errno;
	(void)close(fd);

	return res == 0 ? 0 : chardev_backconvert_errno(saved_errno);
}

static int (*get_sysparm_fn)(unsigned int parameter, unsigned int length, char *data);
static int (*set_sysparm_fn)(unsigned int parameter, char *data);

static void sysparm_fn_setup(void)
{
	const bool use_chardev = sysparm_can_use_chardev();

	get_sysparm_fn = use_chardev ?
		get_sysparm_chardev : get_sysparm_syscall_fallback;
	set_sysparm_fn = use_chardev ?
		set_sysparm_chardev : set_sysparm_syscall_fallback;
}

static pthread_once_t sysparm_fn_setup_once = PTHREAD_ONCE_INIT;

int rtas_get_sysparm(unsigned int parameter, unsigned int length, char *data)
{
	pthread_once(&sysparm_fn_setup_once, sysparm_fn_setup);
	return get_sysparm_fn(parameter, length, data);
}

int rtas_set_sysparm(unsigned int parameter, char *data)
{
	pthread_once(&sysparm_fn_setup_once, sysparm_fn_setup);
	return set_sysparm_fn(parameter, data);
}
