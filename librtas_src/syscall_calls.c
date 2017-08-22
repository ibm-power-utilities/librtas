/**
 * @file syscall_calls.c
 *
 * Provide user space rtas functions that go through the RTAS system call.
 *
 * Copyright (C) 2005 IBM Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include "syscall.h"
#include "librtas.h"

/* The original librtas used the _syscall1 interface to get to the rtas
 * system call.  On recent versions of Linux though the _syscall1
 * interface was removed from unistd.h so we have moved to using the
 * syscall() interface instead.  The use of _syscall1 is left as the
 * default to avoid breaking the library on older systems.
 */
#ifdef _syscall1
_syscall1(int, rtas, void *, args);
#endif

#define CALL_AGAIN 1

int dbg_lvl = 0;
static uint64_t rtas_timeout_ms;

/**
 * sanity_check
 * @brief validate the caller credentials and rtas interface
 *
 * @return 0 for success, !o on failure
 */
int sanity_check(void)
{
	if (geteuid() != (uid_t)0)
		return RTAS_PERM;

	if (!interface_exists())
		return RTAS_KERNEL_INT;

	return 0;
}

/**
 * handle_delay
 * @brief sleep for the specified delay time
 *
 * @param status
 * @param elapsed
 * @return
 *	0 		if the status isn't delay-related
 *	CALL_AGAIN	if the status is delay related
 *	RTAS_TIMEOUT if the requested timeout has been exceeded
 */
static unsigned int handle_delay(int status, uint64_t * elapsed)
{
	int order = status - EXTENDED_DELAY_MIN;
	unsigned long ms = 0;
	uint64_t remaining;

	if (status >= EXTENDED_DELAY_MIN && status <= EXTENDED_DELAY_MAX) {
		/* Extended Delay */
		for (ms = 1; order > 0; order--)
			ms = ms * 10;
	} else if (status == RC_BUSY) {
		/* Regular Delay */
		ms = 1;
	} else {
		/* Not a delay return code */
		return 0;
	}

	if (rtas_timeout_ms) {
		if (*elapsed >= rtas_timeout_ms)
			return RTAS_TIMEOUT;

		remaining = rtas_timeout_ms - *elapsed;
		if (ms > remaining)
			ms = remaining;
	}
	*elapsed += ms;

	dbg("Return status %d, delaying for %ld ms\n", status, ms);
	usleep(ms * 1000);
	return 1;
}

/**
 * display_rtas_buf
 * @brief Dump the contents of the rtas call buffer
 *
 * @param args
 * @param after
 */
static void display_rtas_buf(struct rtas_args *args, int after)
{
	int i, ninputs, nret;

	if (dbg_lvl < 2)
		return;

	ninputs = be32toh(args->ninputs);
	nret = be32toh(args->nret);

	/* It doesn't make sense to byte swap the input and return arguments
	 * as we don't know here what they really mean (it could be a 64 bit
	 * value split accross two 32 bit arguments). Printing them as hex
	 * values is enough for debugging purposes.
	 */
	if (!after) {
		printf("RTAS call args.token = %d\n", be32toh(args->token));
		printf("RTAS call args.ninputs = %d\n", ninputs);
		printf("RTAS call args.nret = %d\n", nret);
		for (i = 0; i < ninputs; i++)
			printf("RTAS call input[%d] = 0x%x (BE)\n", i,
			       args->args[i]);
	} else {
		for (i = ninputs; i < ninputs + nret; i++)
			printf("RTAS call output[%d] = 0x%x (BE)\n",
			       i - ninputs, args->args[i]);
	}
}

/**
 * rtas_call
 * @brief Perform the actual  system call for the rtas call
 *
 * Variable argument list consists of inputs followed by
 * pointers to outputs.
 *
 * @param token
 * @param ninputs number of inputs
 * @param nret number of return variables
 * @return 0 on success, !0 otherwise
 */
static int _rtas_call(int delay_handling, int token, int ninputs,
		      int nrets, va_list *ap)
{
	struct rtas_args args;
	rtas_arg_t *rets[MAX_ARGS];
	uint64_t elapsed = 0;
	int i, rc;

	args.token = htobe32(token);
	args.ninputs = htobe32(ninputs);
	args.nret = htobe32(nrets);

	for (i = 0; i < ninputs; i++)
		args.args[i] = (rtas_arg_t) va_arg(*ap, unsigned long);

	for (i = 0; i < nrets; i++)
		rets[i] = (rtas_arg_t *) va_arg(*ap, unsigned long);

	display_rtas_buf(&args, 0);

	do {
#ifdef _syscall1
		rc = rtas(&args);
#else
		rc = syscall(__NR_rtas, &args);
#endif
		if (!delay_handling || (rc < 0))
			break;

		rc = handle_delay(be32toh(args.args[ninputs]), &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc != 0) {
		dbg("RTAS syscall failure, errno=%d\n", errno);
		return RTAS_IO_ASSERT;
	}

	display_rtas_buf(&args, 1);

	/* Assign rets */
	if (nrets) {
		/* All RTAS calls return a status in rets[0] */
		*(rets[0]) = be32toh(args.args[ninputs]);

		for (i = 1; i < nrets; i++)
			*(rets[i]) = args.args[ninputs + i];
	}

	return 0;
}

static int rtas_call_no_delay(const char *name, int ninputs, int nrets, ...)
{
	va_list ap;
	int rc, token;

	token = rtas_token(name);
	if (token < 0)
		return token;

	va_start(ap, nrets);
	rc = _rtas_call(0, token, ninputs, nrets, &ap);
	va_end(ap);

	return rc;
}

static int rtas_call(const char *name, int ninputs, int nrets, ...)
{
	va_list ap;
	int rc, token;

	token = rtas_token(name);
	if (token < 0)
		return token;

	va_start(ap, nrets);
	rc = _rtas_call(1, token, ninputs, nrets, &ap);
	va_end(ap);

	return rc;
}

/**
 * rtas_activate_firmware
 * @brief Interface for ibm,activate-firmware rtas call
 *
 * @return 0 on success, !0 on failure
 */
int rtas_activate_firmware()
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("ibm,activate-firmware", 0, 1, &status);

	dbg("() = %d\n", rc ? rc : status);
	return rc ? rc : status;
}

#define CFG_RC_DONE 0
#define CFG_RC_MEM 5

/**
 * rtas_cfg_connector
 * @brief Interface for ibm,configure-connector rtas call
 *
 * @param workarea buffer containg args to ibm,configure-connector
 * @return 0 on success, !0 on failure
 */
int rtas_cfg_connector(char *workarea)
{
	uint32_t workarea_pa;
	uint32_t extent_pa = 0;
	uint64_t elapsed = 0;
	void *kernbuf;
	void *extent;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(PAGE_SIZE, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, PAGE_SIZE);

	do {
		rc = rtas_call_no_delay("ibm,configure-connector", 2, 1,
					htobe32(workarea_pa),
					htobe32(extent_pa), &status);
		if (rc < 0)
			break;

		if ((rc == 0) && (status == CFG_RC_MEM)) {
			rc = rtas_get_rmo_buffer(PAGE_SIZE, &extent,
						 &extent_pa);
			if (rc < 0)
				break;

			continue;
		}

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, PAGE_SIZE);

	(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	if (extent_pa)
		(void)rtas_free_rmo_buffer(extent, extent_pa, PAGE_SIZE);

	dbg("(%p) = %d\n", workarea, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_delay_timeout
 * @brief Interface to retrieve the rtas timeout delay
 *
 * @param timeout_ms timeout in milli-seconds
 * @return delay time
 */
int rtas_delay_timeout(uint64_t timeout_ms)
{
	int rc;

	rc = sanity_check();
	if (rc)
		return rc;

	rtas_timeout_ms = timeout_ms;

	return 0;
}

/**
 * rtas_display_char
 * @brief Interface for display-character rtas call
 *
 * @param c character to display
 * @return 0 on success, !0 otherwise
 */
int rtas_display_char(char c)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("display-character", 1, 1, c, &status);

	dbg("(%d) = %d\n", c, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_display_msg
 * @brief Interface for ibm,display-message rtas call
 *
 * @param buf message to display
 * @return 0 on success, !0 otherwise
 */
int rtas_display_msg(char *buf)
{
	uint32_t kernbuf_pa;
	void *kernbuf;
	int str_len;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	str_len = strlen(buf);

	rc = rtas_get_rmo_buffer(str_len, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	strcpy(kernbuf, buf);

	rc = rtas_call("ibm,display-message", 1, 1, htobe32(kernbuf_pa),
		       &status);
	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, str_len);

	dbg("(%p) = %d\n", buf, rc ? rc : status);
	return rc ? rc : status;
}

#define ERRINJCT_BUF_SIZE 1024

/**
 * rtas_errinjct
 * @brief Interface to the ibm,errinjct rtas call
 *
 * @param etoken errinjct token
 * @param otoken errinjct open token
 * @param workarea additional args to ibm,errinjct
 * @return 0 on success, !0 otherwise
 */
int rtas_errinjct(int etoken, int otoken, char *workarea)
{
	uint32_t kernbuf_pa;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(ERRINJCT_BUF_SIZE, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, ERRINJCT_BUF_SIZE);

	rc = rtas_call("ibm,errinjct", 3, 1, htobe32(etoken), htobe32(otoken),
		       htobe32(kernbuf_pa), &status);

	if (rc == 0)
		memcpy(workarea, kernbuf, ERRINJCT_BUF_SIZE);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, ERRINJCT_BUF_SIZE);

	dbg("(%d, %d, %p) = %d\n", etoken, otoken, workarea, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_errinjct_close
 * @brief Inerface to close the ibm,errinjct facility
 *
 * @param otoken errinjct open token
 * @return 0 on success, !0 otherwise
 */
int rtas_errinjct_close(int otoken)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("ibm,close-errinjct", 1, 1, htobe32(otoken), &status);

	dbg("(%d) = %d\n", otoken, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_errinjct_open
 * @brief Interface to open the ibm,errinjct facility
 *
 * This call will set the value refrenced by otoken to the open token
 * for the ibm,errinjct facility
 *
 * @param otoken pointer to open token
 * @return 0 on success, !0 otherwise
 */
int rtas_errinjct_open(int *otoken)
{
	__be32 be_status;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	/*
	 * Unlike other RTAS calls, here first output parameter is otoken,
	 * not status. rtas_call converts otoken to host endianess. We
	 * have to convert status parameter.
	 */
	rc = rtas_call("ibm,open-errinjct", 0, 2, otoken, &be_status);
	status = be32toh(be_status);

	dbg("(%p) = %d, %d\n", otoken, rc ? rc : status, *otoken);
	return rc ? rc : status;
}

/**
 * rtas_get_config_addr_info2
 * @brief Interface to ibm,get-config-addr-info2 rtas call
 *
 * On successful completion the info value is returned.
 *
 * @param config_addr
 * @param phb_unit_id
 * @param func
 * @param info
 * @return 0 on success, !0 otherwise
 */
int rtas_get_config_addr_info2(uint32_t config_addr, uint64_t phb_id,
			       uint32_t func, uint32_t *info)
{
	__be32 be_info;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("ibm,get-config-addr-info2", 4, 2, htobe32(config_addr),
		       htobe32(BITS32_HI(phb_id)), htobe32(BITS32_LO(phb_id)),
		       htobe32(func), &status, &be_info);

	*info = be32toh(be_info);

	dbg("(0x%x, 0x%lx, %d) = %d, 0x%x\n", config_addr, phb_id, func,
	    rc ? rc : status, *info);
	return rc ? rc : status;
}

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
int rtas_get_dynamic_sensor(int sensor, void *loc_code, int *state)
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
int rtas_get_indices(int is_sensor, int type, char *workarea, size_t size,
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

	dbg("(%d, %d, %p, %zd, %d, %p) = %d, %d\n", is_sensor, type, workarea,
	     size, start, next, rc ? rc : status, *next);
	return rc ? rc : status;
}

/**
 * rtas_get_power_level
 * @brief Interface to the get-power-level rtas call
 *
 * On success this routine will set the variable referenced by the level
 * parameter to the power level
 *
 * @param powerdomain
 * @param level reference to the power level variable
 * @return 0 on success, !0 otherwise
 */
int rtas_get_power_level(int powerdomain, int *level)
{
	__be32 be_level;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("get-power-level", 1, 2, htobe32(powerdomain),
		       &status, &be_level);

	*level = be32toh(be_level);

	dbg("(%d, %p) = %d, %d\n", powerdomain, level, rc ? rc : status,
	    *level);
	return rc ? rc : status;
}

/**
 * rtas_get_sensor
 * @brief Interface to the get-sensor-state rtas call
 *
 * On successful completion the state parameter will reference the current
 * state of the sensor
 *
 * @param sensor
 * @param index sensor index
 * @param state reference to state variable
 * @return 0 on success, !0 otherwise
 */
int rtas_get_sensor(int sensor, int index, int *state)
{
	__be32 be_state;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("get-sensor-state", 2, 2, htobe32(sensor),
		       htobe32(index), &status, &be_state);

	*state = be32toh(be_state);

	dbg("(%d, %d, %p) = %d, %d\n", sensor, index, state, rc ? rc : status,
	    *state);
	return rc ? rc : status;
}

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

	dbg("(%d, %d, %p) = %d\n", parameter, length, data, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_get_time
 * @brief Interface to get-time-of-day rtas call
 *
 * On successful completion all of the parameters will be filled with
 * their respective values for the current time of day.
 *
 * @param year
 * @param month
 * @param day
 * @param hour
 * @param min
 * @param sec
 * @param nsec
 * @return 0 on success, !0 otherwise
 */
int rtas_get_time(uint32_t *year, uint32_t *month, uint32_t *day,
		  uint32_t *hour, uint32_t *min, uint32_t *sec, uint32_t *nsec)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("get-time-of-day", 0, 8, &status, year, month, day,
		       hour, min, sec, nsec);

	*year = be32toh(*year);
	*month = be32toh(*month);
	*day = be32toh(*day);
	*hour = be32toh(*hour);
	*min = be32toh(*min);
	*sec = be32toh(*sec);
	*nsec = be32toh(*nsec);

	dbg("() = %d, %d, %d, %d, %d, %d, %d, %d\n", rc ? rc : status, *year,
	    *month, *day, *hour, *min, *sec, *nsec);
	return rc ? rc : status;
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

	rc = rtas_get_rmo_buffer(size + PAGE_SIZE, &rmobuf, &rmo_pa);
	if (rc)
		return rc;

	kernbuf = rmobuf + PAGE_SIZE;
	kernbuf_pa = rmo_pa + PAGE_SIZE;
	locbuf = rmobuf;
	loc_pa = rmo_pa;

	/* If user didn't set loc_code, copy a NULL string */
	strncpy(locbuf, loc_code ? loc_code : "", PAGE_SIZE);

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

	(void) rtas_free_rmo_buffer(rmobuf, rmo_pa, size + PAGE_SIZE);

	*seq_next = be32toh(*seq_next);
	*bytes_ret = be32toh(*bytes_ret);

	dbg("(%s, 0x%p, %zd, %d) = %d, %d, %d", loc_code ? loc_code : "NULL",
	    workarea, size, sequence, status, *seq_next, *bytes_ret);
	return rc ? rc : status;
}

/**
 * rtas_lpar_perftools
 * @brief Interface to the ibm,lpa-perftools rtas call
 *
 * @param subfunc
 * @param workarea additional args to the rtas call
 * @param length
 * @param sequence
 * @param seq_next
 * @return 0 on success, !0 otherwise
 */
int rtas_lpar_perftools(int subfunc, char *workarea, unsigned int length,
			unsigned int sequence, unsigned int *seq_next)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, PAGE_SIZE);

	*seq_next = htobe32(sequence);
	do {
		sequence = *seq_next;
		rc = rtas_call_no_delay("ibm,lpar-perftools", 5, 2,
					htobe32(subfunc), 0,
					htobe32(kernbuf_pa), htobe32(length),
					sequence, &status, seq_next);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, length);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	*seq_next = be32toh(*seq_next);

	dbg("(%d, %p, %d, %d, %p) = %d, %d\n", subfunc, workarea,
	    length, sequence, seq_next, rc ? rc : status, *seq_next);
	return rc ? rc : status;
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
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa = 0;
	uint32_t next_hi, next_lo;
	uint32_t bytes_hi, bytes_lo;
	uint32_t dump_tag_hi, dump_tag_lo;
	void *kernbuf;
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

	dbg("(0x%lx, 0x%lx, %p, %zd, %p, %p) = %d, 0x%lx, 0x%lx\n",
	     dump_tag, sequence, buffer, length, seq_next, bytes_ret,
	     rc ? rc : status, *seq_next, *bytes_ret);
	return rc ? rc : status;
}

/**
 * rtas_read_slot_reset
 * @brief Interface to the ibm,read-slot-reset-state rtas call
 *
 * @param cfg_addr configuration address of slot to read
 * @param phbid PHB ID of the slot to read
 * @param state reference to variable to return slot state in
 * @param eeh
 * @return 0 on success, !0 otherwise
 */
int rtas_read_slot_reset(uint32_t cfg_addr, uint64_t phbid, int *state,
			 int *eeh)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("ibm,read-slot-reset-state", 3, 3, htobe32(cfg_addr),
		       htobe32(BITS32_HI(phbid)), htobe32(BITS32_LO(phbid)),
		       &status, state, eeh);

	*state = be32toh(*state);
	*eeh = be32toh(*eeh);

	dbg("(0x%x, 0x%lx, %p, %p) = %d, %d, %d\n", cfg_addr, phbid, state,
	     eeh, rc ? rc : status, *state, *eeh);
	return rc ? rc : status;
}

/**
 * rtas_scan_log_dump
 * @brief Interface to the ibm,scan-log-dump rtas call
 *
 * @param buffer buffer to return scan log dump in
 * @param length size of buffer
 * @return 0 on success, !0 otherwise
 */
int rtas_scan_log_dump(void *buffer, size_t length)
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

	memcpy(kernbuf, buffer, length);
	rc = rtas_call("ibm,scan-log-dump", 2, 1, htobe32(kernbuf_pa),
		       htobe32(length), &status);

	if (rc == 0)
		memcpy(buffer, kernbuf, length);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	dbg("(%p, %zd) = %d\n", buffer, length, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_set_debug
 * @brief Interface to set librtas debug level
 *
 * @param level debug level to set to
 * @return 0 on success, !0 otherwise
 */
int rtas_set_debug(int level)
{
	dbg_lvl = level;
	return 0;
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
 * rtas_set_eeh_option
 * @brief Inerface to the ibm,set-eeh-option rtas call
 *
 * @param cfg_addr configuration address for slot to set eeh option on
 * @param phbid PHB ID for slot to set option on
 * @param function
 * @return 0 on success, !0 otherwise
 */
int rtas_set_eeh_option(uint32_t cfg_addr, uint64_t phbid, int function)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("ibm,set-eeh-option", 4, 1, htobe32(cfg_addr),
		       htobe32(BITS32_HI(phbid)), htobe32(BITS32_LO(phbid)),
		       htobe32(function), &status);

	dbg("(0x%x, 0x%lx, %d) = %d\n", cfg_addr, phbid, function,
	     rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_set_indicator
 * @brief Interface to the set-indicator rtas call
 *
 * @param indicator indicator to set
 * @param index indicator index
 * @param new_value value to set the indicator to
 * @return 0 on success, !0 otherwise
 */
int rtas_set_indicator(int indicator, int index, int new_value)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("set-indicator", 3, 1, htobe32(indicator),
		       htobe32(index), htobe32(new_value), &status);

	dbg("(%d, %d, %d) = %d\n", indicator, index, new_value,
	    rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_set_power_level
 * @brief Interface to the set-power-level rtas call
 *
 * @param powerdomain
 * @param level power level to set to
 * @param setlevel
 * @return 0 on success, !0 otherwise
 */
int rtas_set_power_level(int powerdomain, int level, int *setlevel)
{
	__be32 be_setlevel;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("set-power-level", 2, 2, htobe32(powerdomain),
		       htobe32(level), &status, &be_setlevel);

	*setlevel = be32toh(be_setlevel);

	dbg("(%d, %d, %p) = %d, %d\n", powerdomain, level, setlevel,
	    rc ? rc : status, *setlevel);
	return rc ? rc : status;
}

/**
 * rtas_set_poweron_time
 * @brief interface to the set-time-for-power-on rtas call
 *
 * @param year year to power on
 * @param month month to power on
 * @param day day to power on
 * @param hour hour to power on
 * @param min minute to power on
 * @param sec second to power on
 * @param nsec nano-second top power on
 * @return 0 on success, !0 otherwise
 */
int rtas_set_poweron_time(uint32_t year, uint32_t month, uint32_t day,
			  uint32_t hour, uint32_t min, uint32_t sec,
			  uint32_t nsec)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("set-time-for-power-on", 7, 1, htobe32(year),
		       htobe32(month), htobe32(day), htobe32(hour),
		       htobe32(min), htobe32(sec), htobe32(nsec), &status);

	dbg("(%d, %d, %d, %d, %d, %d, %d) = %d\n", year, month, day, hour,
	    min, sec, nsec, rc ? rc : status);
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
	short size;

	rc = sanity_check();
	if (rc)
		return rc;

	size = *(short *)data;
	rc = rtas_get_rmo_buffer(size + sizeof(short), &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, data, size + sizeof(short));

	rc = rtas_call("ibm,set-system-parameter", 2, 1, htobe32(parameter),
		       htobe32(kernbuf_pa), &status);

	(void)rtas_free_rmo_buffer(kernbuf, kernbuf_pa, size + sizeof(short));

	dbg("(%d, %p) = %d\n", parameter, data, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_set_time
 * @brief Interface to the set-time-of-day rtas call
 *
 * @param year year to set time to
 * @param month month to set time to
 * @param day day to set time to
 * @param hour hour to set time to
 * @param min minute to set time to
 * @param sec second to set time to
 * @param nsec nan-second to set time to
 * @return 0 on success, !0 otherwise
 */
int rtas_set_time(uint32_t year, uint32_t month, uint32_t day, uint32_t hour,
		  uint32_t min, uint32_t sec, uint32_t nsec)
{
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_call("set-time-of-day", 7, 1, htobe32(year), htobe32(month),
		       htobe32(day), htobe32(hour), htobe32(min),
		       htobe32(sec), htobe32(nsec), &status);

	dbg("(%d, %d, %d, %d, %d, %d, %d) = %d\n", year, month, day, hour,
	    min, sec, nsec, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_suspend_me
 * @brief Interface for ibm,suspend-me rtas call
 *
 * @return 0 on success, !0 on failure
 */
int rtas_suspend_me(uint64_t streamid)
{
	int rc, status;

	rc = rtas_call("ibm,suspend-me", 2, 1, htobe32(BITS32_HI(streamid)),
		       htobe32(BITS32_LO(streamid)), &status);

	dbg("() = %d\n", rc ? rc : status);
	return rc ? rc : status;
}

/**
 * rtas_update_nodes
 * @brief Interface for ibm,update-nodes rtas call
 *
 * @param workarea input output work area for the rtas call
 * @param scope of call
 * @return 0 on success, !0 on failure
 *
 * Note that the PAPR defines the work area as 4096 bytes (as
 * opposed to a page), thus we use that rather than PAGE_SIZE below.
 */
int rtas_update_nodes(char *workarea, unsigned int scope)
{
	uint32_t workarea_pa;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(4096, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, 4096);

	rc = rtas_call("ibm,update-nodes", 2, 1, htobe32(workarea_pa),
		       htobe32(scope), &status);

	if (rc == 0)
		memcpy(workarea, kernbuf, 4096);

	(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	dbg("(%p) %d = %d\n", workarea, scope, rc ? rc : status);
	return rc ? rc : status;
}

 /**
 * rtas_update_properties
 * @brief Interface for ibm,update-properties rtas call
 *
 * @param workarea input output work area for the rtas call
 * @param scope of call
 * @return 0 on success, !0 on failure
 *
 * Note that the PAPR defines the work area as 4096 bytes (as
 * opposed to a page), thus we use that rather than PAGE_SIZE below.
 */
int rtas_update_properties(char *workarea, unsigned int scope)
{
	uint32_t workarea_pa;
	void *kernbuf;
	int rc, status;

	rc = sanity_check();
	if (rc)
		return rc;

	rc = rtas_get_rmo_buffer(4096, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, 4096);

	rc = rtas_call("ibm,update-properties", 2, 1, htobe32(workarea_pa),
		       htobe32(scope), &status);

	if (rc == 0)
		memcpy(workarea, kernbuf, 4096);

	(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	dbg("(%p) %d = %d\n", workarea, scope, rc ? rc : status);
	return rc ? rc : status;
}

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
	int kbuf_sz = 4096;
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
	if (be32toh(resp_bytes) > *work_area_bytes) {
		(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, kbuf_sz);
		return RTAS_IO_ASSERT;
	}

	*work_area_bytes = be32toh(resp_bytes);

	if (rc == 0)
		memcpy(workarea, kernbuf, *work_area_bytes);

	(void)rtas_free_rmo_buffer(kernbuf, workarea_pa, kbuf_sz);

	return rc ? rc : status;
}
