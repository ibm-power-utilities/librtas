// SPDX-License-Identifier: LGPL-2.1-or-later

// Support for accessing IBM Power systems indices (indicator and sensor)
// data via /dev/papr-indices or the legacy rtas() syscalls.

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "internal.h"
#include "librtas.h"
#include "papr-indices.h"

static const char indices_devpath[] = "/dev/papr-indices";

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
int get_dynamic_sensor_fallback(int sensor, void *loc_code, int *state)
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
int get_indices_fallback(int is_sensor, int type, char *workarea, size_t size,
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

static int get_indices_fd_new(int is_sensor, int type)
{
	struct papr_indices_io_block buf = {};
	const int fd = open(indices_devpath, O_WRONLY);
	int devfd = -1;

	if (fd < 0)
		return -1;

	buf.indices.is_sensor = is_sensor;
	buf.indices.indice_type = type;
	devfd = ioctl(fd, PAPR_INDICES_IOC_GET, &buf);
	close(fd);

	return devfd;
}

static int get_indices_chardev(int is_sensor, int type, char *workarea,
				size_t size, int start, int *next)
{
	int fd, rtas_status = 0;
	ssize_t res;

	if (size != RTAS_GET_INDICES_BUF_SIZE) {
		dbg("Invalid buffer size %lu expects %d\n",
				size, RTAS_GET_INDICES_BUF_SIZE);
		return -EINVAL;
	}

	fd = (start == 1) ? get_indices_fd_new(is_sensor, type)
				: (int)start;
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

	res = read(fd, workarea, size);
	if (res < 0) {
		/* Synthesize ibm,get-platfrom-dump "hardware error" */
		rtas_status = -1;
		close(fd);
	} else if (res == 0) {
		/*
		 * read() returns 0 at the end of read
		 * So reset the first 32 bit value (number of indices)
		 * in the buffer which tells no data available to the
		 * caller of rtas_get_indices().
		 */
		*(uint32_t *)workarea = 0;
		rtas_status = 0; /* Done with sequence, no more data */
		close(fd);
		if (next)
			*next = 1;
	} else {
		rtas_status = 1; /* More data available, call again */
		if (next)
			*next = fd;
	}

	return rtas_status;
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

static int dynamic_common_io_setup(unsigned long ioctalval,
				void *loc_code,
				struct papr_indices_io_block *buf)
{
	size_t length;
	char *loc_str;
	int fd, ret = -EINVAL;

	fd = open(indices_devpath, O_RDWR);
	if (fd < 0) {
		/*
		 * Should not be here. May be /dev/papr-indices removed
		 */
		return -1;
	}

	length = be32toh(*(uint32_t *)loc_code);

	if (length < 1) {
		dbg("Invalid length(%lu) of location code string\n", length);
		goto out;
	}

	loc_str = (char *)((char *)loc_code + sizeof(uint32_t));
	if (strlen(loc_str) != (length - 1)) {
		dbg("location code string length is not matched with the passed length(%lu)\n", length);
		goto out;
	}

	memcpy(&buf->dynamic_param.location_code_str, loc_str, length);

	ret = ioctl(fd, ioctalval, buf);
	if (ret != 0)
		ret = chardev_backconvert_errno(errno);
out:
	close(fd);
	return ret;
}

static int get_dynamic_sensor_chardev(int sensor, void *loc_code, int *state)
{
	struct papr_indices_io_block buf = {};
	int ret;

	buf.dynamic_param.token = sensor;
	ret = dynamic_common_io_setup(PAPR_DYNAMIC_SENSOR_IOC_GET,
				loc_code, &buf);
	if (ret != 0)
		return ret;

	*state = buf.dynamic_param.state;

	return 0;
}

static bool indices_can_use_chardev(void)
{
	struct stat statbuf;

	if (stat(indices_devpath, &statbuf))
		return false;

	if (!S_ISCHR(statbuf.st_mode))
		return false;

	if (close(open(indices_devpath, O_RDONLY)))
		return false;

	return true;
}

static int (*get_indices_fn)(int is_sensor, int type, char *workarea,
				size_t size, int start, int *next);
static int (*get_dynamic_sensor_fn)(int sensor, void *loc_code, int *state);

static void indices_fn_setup(void)
{
	const bool use_chardev = indices_can_use_chardev();

	get_indices_fn = use_chardev ?
		get_indices_chardev : get_indices_fallback;
	get_dynamic_sensor_fn = use_chardev ?
		get_dynamic_sensor_chardev : get_dynamic_sensor_fallback;
}

static pthread_once_t indices_fn_setup_once = PTHREAD_ONCE_INIT;

int rtas_get_indices(int is_sensor, int type, char *workarea, size_t size,
			int start, int *next)
{
	pthread_once(&indices_fn_setup_once, indices_fn_setup);
	return get_indices_fn(is_sensor, type, workarea, size,
				start, next);
}

int rtas_get_dynamic_sensor(int sensor, void *loc_code, int *state)
{
	pthread_once(&indices_fn_setup_once, indices_fn_setup);
	return get_dynamic_sensor_fn(sensor, loc_code, state);
}
