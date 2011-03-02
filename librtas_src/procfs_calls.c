/**
 * @file procfs_calls.c
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "procfs.h"
#include "librtas.h"

#define CFG_CONN_FILE "cfg_connector"
#define GET_POWER_FILE "get_power"
#define GET_SENSOR_FILE "get_sensor"
#define GET_SYSPARM_FILE "get_sysparm"
#define SET_INDICATOR_FILE "set_indicator"
#define SET_POWER_FILE "set_power"

int pfs_cfg_connector(int token, char *workarea);
int pfs_get_power(int token, int domain, int *level);
int pfs_get_sensor(int token, int sensor, int index, int *state);
int pfs_get_sysparm(int token, unsigned int parameter, unsigned int length,
		    char *data);
int pfs_set_indicator(int token, int indicator, int index, int new_value);
int pfs_set_power(int token, int domain, int level, int *setlevel);

int procfs_interface_exists();

struct rtas_operations procfs_rtas_ops = {
	.cfg_connector = pfs_cfg_connector,
	.get_power_level = pfs_get_power,
	.get_sensor = pfs_get_sensor,
	.get_sysparm = pfs_get_sysparm,
	.set_indicator = pfs_set_indicator,
	.set_power_level = pfs_set_power,
	.interface_exists = procfs_interface_exists,
};

/**
 * do_rtas_op
 * @brief Perform the actual rtas call via the supplied procfs file
 *
 * @param proc_filename procfs filename for the rtas call
 * @param buf buffer to read rtas call results into
 * @param buf_size size of the buf
 * @return 0 on success, !0 otherwise
 */
static int do_rtas_op(const char *proc_filename, void *buf, size_t buf_size)
{
	int rc;
	int fd;

	/* Open /proc file */
	fd = open_proc_rtas_file(proc_filename, O_RDWR);
	if (fd <= 0)
		return RTAS_KERNEL_IMP;

	/* Write RTAS call args buf */
	rc = write(fd, buf, buf_size);
	if (rc < (int)buf_size) {
		fprintf(stderr, "Failed to write proc file %s\n",
			proc_filename);
		return RTAS_IO_ASSERT;
	}

	/* Read results */
	rc = read(fd, buf, buf_size);
	if (rc < (int)buf_size) {
		fprintf(stderr, "Failed to read proc file %s\n", proc_filename);
		return RTAS_IO_ASSERT;
	}

	/* Close /proc file */
	close(fd);

	return 0;
}

/**
 * pfs_cfg_connector
 * @brief Set up arguments for procfs cfg-connector rtas call
 *
 * @param token
 * @param workarea
 * @return 0 on success, !0 otherwise
 */
int pfs_cfg_connector(int token, char *workarea)
{
	struct rtas_cfg_connector args;
	int rc;

	memcpy(&args.workarea[0], workarea, PAGE_SIZE);
	rc = do_rtas_op(CFG_CONN_FILE, &args,
			sizeof(struct rtas_cfg_connector));
	if (rc)
		return rc;

	memcpy(workarea, &args.workarea[0], PAGE_SIZE);

	return args.status;
}

/**
 * pfs_get_power
 * @brief Set up args for procfs get-power-level rtas call
 *
 * @param token
 * @param domain
 * @param level
 * @return 0 on success, !0 otherwise
 */
int pfs_get_power(int token, int domain, int *level)
{
	struct rtas_get_power_level args;
	int rc;

	args.powerdomain = domain;

	rc = do_rtas_op(GET_POWER_FILE, &args,
			sizeof(struct rtas_get_power_level));
	if (rc)
		return rc;

	*level = args.level;

	return args.status;
}

/**
 * pfs_get_sensor
 * @brief Set up args for procfs get-sensor-state rtas call
 *
 * @param token
 * @param sensor
 * @param index
 * @param state
 * @return 0 on success, !0 otherwise
 */
int pfs_get_sensor(int token, int sensor, int index, int *state)
{
	struct rtas_get_sensor args;
	int rc;

	args.sensor = sensor;
	args.index = index;

	rc = do_rtas_op(GET_SENSOR_FILE, &args, sizeof(struct rtas_get_sensor));
	if (rc)
		return rc;

	*state = args.state;

	return args.status;
}

/**
 * pfs_get_sysparm
 * @brief Set up call to procfs get-system-parameter rtas call
 *
 * @param token
 * @param parameter
 * @param length
 * @param data
 * @return 0 on success, !0 otherwise
 */
int pfs_get_sysparm(int token, unsigned int parameter, unsigned int length,
		    char *data)
{
	struct rtas_get_sysparm args;
	int rc;

	args.parameter = parameter;
	args.length = length;
	memcpy(&args.data[0], data, PAGE_SIZE);

	rc = do_rtas_op(GET_SYSPARM_FILE, &args,
			sizeof(struct rtas_get_sysparm));

	if (args.status == 0)
		memcpy(data, &args.data[0], PAGE_SIZE);

	return args.status;
}

/**
 * pfs_set_indicator
 * @brief Set up call to procfs set-indicator rtas call
 *
 * @param token
 * @param indicator
 * @param index
 * @param new_value
 * @return 0 on success, !0 otherwise
 */ 
int pfs_set_indicator(int token, int indicator, int index, int new_value)
{
	struct rtas_set_indicator args;
	int rc;

	args.indicator = indicator;
	args.index = index;
	args.new_value = new_value;

	rc = do_rtas_op(SET_INDICATOR_FILE, &args,
			sizeof(struct rtas_set_indicator));
	if (rc)
		return rc;

	return args.status;
}

/**
 * pfs_set_power
 *
 * @param token
 * @param domain
 * @param level
 * @param setlevel
 * @return 0 on success, !0 otherwise
 */
int pfs_set_power(int token, int domain, int level, int *setlevel)
{
	struct rtas_set_power_level args;
	int rc;

	args.powerdomain = domain;
	args.level = level;

	rc = do_rtas_op(SET_POWER_FILE, &args,
			sizeof(struct rtas_set_power_level));
	if (rc)
		return rc;

	*setlevel = args.setlevel;

	return args.status;
}

/**
 * procfs_interfac_exists
 * @brief VAlidate that the procfs interface to rtas exists
 *
 * @return 0 on success, !0 otherwise
 */
int procfs_interface_exists()
{
	int fd;
	int exists;

	fd = open_proc_rtas_file("get_sensor", O_RDWR);
	exists = (fd >= 0);

	if (exists)
		close(fd);

	return exists;
}
