/**
 * @file syscall_calls.c
 *
 * Provide user space rtas functions that go through the RTAS system call.
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include "common.h"
#include "syscall.h"
#include "librtas.h"

int sc_activate_firmware(int token);
int sc_cfg_connector(int token, char *workarea);
int sc_delay_timeout(uint64_t timeout_ms);
int sc_display_char(int token, char c);
int sc_display_msg(int token, char *buf);
int sc_errinjct(int token, int etoken, int otoken, char *workarea);
int sc_errinjct_close(int token, int otoken);
int sc_errinjct_open(int token, int *otoken);
int sc_get_dynamic_sensor(int token, int sensor, void *loc_code, int *state);
int sc_get_config_addr_info2(int token, uint32_t config_addr, uint64_t phb_id, uint32_t func, uint32_t *info);
int sc_get_indices(int token, int is_sensor, int type, char *workarea,
		   size_t size, int start, int *next);
int sc_get_power_level(int token, int powerdomain, int *level);
int sc_get_sensor(int token, int sensor, int index, int *state);
int sc_get_sysparm(int token, unsigned int parameter, unsigned int length,
		   char *data);
int sc_get_time(int token, uint32_t *year, uint32_t *month, uint32_t *day, 
		uint32_t *hour, uint32_t *min, uint32_t *sec, uint32_t *nsec);
int sc_get_vpd(int token, char *loc_code, char *workarea, size_t size, 
		unsigned int sequence, unsigned int *seq_next, 
		unsigned int *bytes_ret);
int sc_lpar_perftools(int token, int subfunc, char *workarea,
		      unsigned int length, unsigned int sequence,
		      unsigned int *seq_next);
int sc_platform_dump(int token, uint64_t dump_tag, uint64_t sequence,
		     void *buffer, size_t length, uint64_t * next_seq,
		     uint64_t * bytes_ret);
int sc_read_slot_reset(int token, uint32_t cfg_addr, uint64_t phbid,
		       int *state, int *eeh);
int sc_scan_log_dump(int token, void *buffer, size_t length);
int sc_set_dynamic_indicator(int token, int indicator, int new_value,
			     void *loc_code);
int sc_set_eeh_option(int token, uint32_t cfg_addr, uint64_t phbid,
		      int function);
int sc_set_indicator(int token, int indicator, int index, int new_value);
int sc_set_power_level(int token, int powerdomain, int level, int *setlevel);
int sc_set_poweron_time(int token, uint32_t year, uint32_t month, uint32_t day,
			uint32_t hour, uint32_t min, uint32_t sec,
			uint32_t nsec);
int sc_set_sysparm(int token, unsigned int parameter, char *data);
int sc_set_time(int token, uint32_t year, uint32_t month, uint32_t day, 
			uint32_t hour, uint32_t min, uint32_t sec, 
			uint32_t nsec);
int sc_suspend_me(int token, uint64_t streamid);
int sc_update_nodes(int token, char *workarea, unsigned int scope);
int sc_update_properties(int token, char *workarea, unsigned int scope);

struct rtas_operations syscall_rtas_ops = {
	.activate_firmware = sc_activate_firmware,
	.cfg_connector = sc_cfg_connector,
	.delay_timeout = sc_delay_timeout,
	.display_char = sc_display_char,
	.display_msg = sc_display_msg,
	.errinjct = sc_errinjct,
	.errinjct_close = sc_errinjct_close,
	.errinjct_open = sc_errinjct_open,
	.free_rmo_buffer = sc_free_rmo_buffer,
	.get_config_addr_info2 = sc_get_config_addr_info2,
	.get_dynamic_sensor = sc_get_dynamic_sensor,
	.get_indices = sc_get_indices,
	.get_sensor = sc_get_sensor,
	.get_power_level = sc_get_power_level,
	.get_rmo_buffer = sc_get_rmo_buffer,
	.get_sysparm = sc_get_sysparm,
	.get_time = sc_get_time,
	.get_vpd = sc_get_vpd,
	.lpar_perftools = sc_lpar_perftools,
	.platform_dump = sc_platform_dump,
	.read_slot_reset = sc_read_slot_reset,
	.scan_log_dump = sc_scan_log_dump,
	.set_dynamic_indicator = sc_set_dynamic_indicator,
	.set_indicator = sc_set_indicator,
	.set_eeh_option = sc_set_eeh_option,
	.set_power_level = sc_set_power_level,
	.set_poweron_time = sc_set_poweron_time,
	.set_sysparm = sc_set_sysparm,
	.set_time = sc_set_time,
	.suspend_me = sc_suspend_me,
	.update_nodes = sc_update_nodes,
	.update_properties = sc_update_properties,
	.interface_exists = sc_interface_exists,
};

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

	if (config.rtas_timeout_ms) {
		if (*elapsed >= config.rtas_timeout_ms)
			return RTAS_TIMEOUT;

		remaining = config.rtas_timeout_ms - *elapsed;
		if (ms > remaining)
			ms = remaining;
	}
	*elapsed += ms;

	dbg1("Return status %d, delaying for %ld ms\n", status, ms);
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

	if (config.debug < 2)
		return;

	ninputs = be32toh(args->ninputs);
	nret    = be32toh(args->nret);

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
 * sc_rtas_call
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
static int sc_rtas_call(int token, int ninputs, int nret, ...)
{
	struct rtas_args args;
	rtas_arg_t *rets[MAX_ARGS];
	va_list ap;
	int rc;
	int i;

	args.token = htobe32(token);
	args.ninputs = htobe32(ninputs);
	args.nret = htobe32(nret);

	va_start(ap, nret);
	for (i = 0; i < ninputs; i++)
		args.args[i] = (rtas_arg_t) va_arg(ap, unsigned long);

	for (i = 0; i < nret; i++)
		rets[i] = (rtas_arg_t *) va_arg(ap, unsigned long);
	va_end(ap);

	display_rtas_buf(&args, 0);

#ifdef _syscall1
	rc = rtas(&args);
#else
	rc = syscall(__NR_rtas, &args);
#endif

	if (rc != 0) {
		dbg1("RTAS syscall failure, errno=%d\n", errno);
		return RTAS_IO_ASSERT;
	}
	display_rtas_buf(&args, 1);

	/* Assign rets */
	if (nret) {
		/* All RTAS calls return a status in rets[0] */
		*(rets[0]) = be32toh(args.args[ninputs]);

		for (i = 1; i < nret; i++)
			*(rets[i]) = args.args[ninputs + i];
	}

	return 0;
}

/**
 * sc_activate_firmware
 * @brief Set up the activate-firmware rtas system call
 *
 * @param token
 * @return 0 on success, !0 otherwise
 */
int sc_activate_firmware(int token)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 0, 1, &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("() = %d\n", rc ? rc : status);
	return rc ? rc : status;
}

#define CFG_RC_DONE 0
#define CFG_RC_MEM 5

/**
 * sc_cfg_connector
 * @brief Set up the cfg-connector rtas system call
 *
 * @param token
 * @param workarea
 * @return 0 on success, !0 otherwise
 */
int sc_cfg_connector(int token, char *workarea)
{
	uint32_t workarea_pa;
	uint32_t extent_pa = 0;
	uint64_t elapsed = 0;
	void *kernbuf;
	void *extent;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(PAGE_SIZE, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, PAGE_SIZE);

	do {
		rc = sc_rtas_call(token, 2, 1, htobe32(workarea_pa),
				  htobe32(extent_pa), &status);
		if (rc < 0)
			break;
		
		if ((rc == 0) && (status == CFG_RC_MEM)) {
			rc = sc_get_rmo_buffer(PAGE_SIZE, &extent, &extent_pa);
			if (rc < 0)
				break;
			continue;
		}

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, PAGE_SIZE);

	(void)sc_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	if (extent_pa)
		(void)sc_free_rmo_buffer(extent, extent_pa, PAGE_SIZE);

	dbg1("(%p) = %d\n", workarea, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_delay_timeout
 * @brief set the configured timeout value
 *
 * @param timeout_ms new timeout in milliseconds
 * @return 0 on success, !0 otherwise
 */
int sc_delay_timeout(uint64_t timeout_ms)
{
	config.rtas_timeout_ms = timeout_ms;

	return 0;
}

/**
 * sc_display_char
 * @brief Set up display-char rtas system call
 * 
 * @param token
 * @param c character to display
 * @return 0 on success, !0 otherwise
 */
int sc_display_char(int token, char c)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 1, 1, c, &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(%d) = %d\n", c, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_display_msg
 * @brief Set up the display-message rtas system call
 *
 * @param token
 * @param buf message to display
 * @return 0 on success, !0 otherwise
 */
int sc_display_msg(int token, char *buf)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int str_len;
	int status;
	int rc;

	str_len = strlen(buf);

	rc = sc_get_rmo_buffer(str_len, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	strcpy(kernbuf, buf);

	do {
		rc = sc_rtas_call(token, 1, 1, htobe32(kernbuf_pa), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while ((rc == CALL_AGAIN));

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, str_len);

	dbg1("(%p) = %d\n", buf, rc ? rc : status);
	return rc ? rc : status;
}

#define ERRINJCT_BUF_SIZE 1024

/**
 * sc_errinjct
 * @brief Set up the errinjct rtas system call
 *
 * @param token
 * @param etoken error injection token
 * @param otoken error injection open token
 * @param workarea additional args to rtas call
 * @return 0 on success, !0 otherwise
 */
int sc_errinjct(int token, int etoken, int otoken, char *workarea)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int status=0;
	int rc;

	rc = sc_get_rmo_buffer(ERRINJCT_BUF_SIZE, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, ERRINJCT_BUF_SIZE);

	do {
		rc = sc_rtas_call(token, 3, 1, htobe32(etoken), htobe32(otoken),
				  htobe32(kernbuf_pa), &status);
		if (rc < 0 )
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, ERRINJCT_BUF_SIZE);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, ERRINJCT_BUF_SIZE);

	dbg1("(%d, %d, %p) = %d\n", etoken, otoken, workarea, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_errinjct_close
 * @brief Set up the errinjct close rtas system call
 *
 * @param token
 * @param otoken errinjct open token
 * @return 0 on success, !0 otherwise
 */
int sc_errinjct_close(int token, int otoken)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 1, 1, htobe32(otoken), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(%d) = %d\n", otoken, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_errinjct_open
 * @brief Set up the errinjct open rtas system call
 *
 * @param token
 * @param otoken
 * @return 0 on success, !0 otherwise
 */
int sc_errinjct_open(int token, int *otoken)
{
	uint64_t elapsed = 0;
	__be32 be_otoken;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 0, 2, &be_otoken, &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*otoken = be32toh(be_otoken);

	dbg1("(%p) = %d, %d\n", otoken, rc ? rc : status, *otoken);
	return rc ? rc : status;
}

/**
 * sc_get_config_addr2
 * @brief get the pci slot config address
 *
 * @param token
 * @param config_addr
 * @param phb_unit_id
 * @param func
 * @param info
 * @return 0 on success, !0 otherwise
 */
int sc_get_config_addr_info2(int token, uint32_t config_addr,
    uint64_t phb_id, uint32_t func, uint32_t *info)
{
	uint64_t elapsed = 0;
	__be32 be_info;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 4, 2, htobe32(config_addr),
				  BITS32_HI(htobe64(phb_id)),
				  BITS32_LO(htobe64(phb_id)),
				  htobe32(func), &status, &be_info);
		if (rc)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*info = be32toh(be_info);

	dbg1("(0x%x, 0x%llx, %d) = %d, 0x%x\n", config_addr, phb_id, func,
	     rc ? rc : status, *info);
	return rc ? rc : status;
}

/**
 * sc_get_dynamic_sensor
 * @brief Set up the get-dynamic-sensor rtas system call
 *
 * @param token
 * @param sensor
 * @param loc_code
 * @param state
 * @return 0 on success, !0 otherwise
 */
int sc_get_dynamic_sensor(int token, int sensor, void *loc_code, int *state)
{
	uint32_t loc_pa = 0;
	uint64_t elapsed = 0;
	void *locbuf;
	uint32_t size;
	__be32 be_state;
	int status;
	int rc;

	size = *(uint32_t *)loc_code + sizeof(uint32_t);

	rc = sc_get_rmo_buffer(size, &locbuf, &loc_pa);
	if (rc)
		return rc;

	memcpy(locbuf, loc_code, size);

	do {
		rc = sc_rtas_call(token, 2, 2, htobe32(sensor), htobe32(loc_pa),
				  &status, &be_state);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	(void) sc_free_rmo_buffer(locbuf, loc_pa, size);

	*state = be32toh(be_state);

	dbg1("(%d, %s, %p) = %d, %d\n", sensor, (char *)loc_code, state,
	     rc ? rc : status, *state);
	return rc ? rc : status;
}

/**
 * sc_get_indices
 * @brief Set up the get-indices rtas system call
 *
 * @param token
 * @param is_sensor
 * @param type
 * @param workarea
 * @param size
 * @param start
 * @param next
 * @return 0 on success, !0 otherwise
 */
int
sc_get_indices(int token, int is_sensor, int type, char *workarea, size_t size,
	       int start, int *next)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	__be32 be_next;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(size, &kernbuf, &kernbuf_pa);
	if (rc) 
		return rc;

	do {
		rc = sc_rtas_call(token, 5, 2, htobe32(is_sensor),
				  htobe32(type), htobe32(kernbuf_pa),
				  htobe32(size), htobe32(start),
				  &status, &be_next);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, size);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, size);

	*next = be32toh(be_next);

	dbg1("(%d, %d, %p, %d, %d, %p) = %d, %d\n", is_sensor, type, workarea,
	     size, start, next, rc ? rc : status, *next);
	return rc ? rc : status;
}

/**
 * sc_get_power_level
 * @brief Set up the get-power-level rtas system call
 *
 * @param token
 * @param powerdomain
 * @param level
 * @return 0 on success, !0 otherwise
 */
int sc_get_power_level(int token, int powerdomain, int *level)
{
	uint64_t elapsed = 0;
	__be32 be_level;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 1, 2, htobe32(powerdomain),
				  &status, &be_level);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*level = be32toh(be_level);

	dbg1("(%d, %p) = %d, %d\n", powerdomain, level, rc ? rc : status,
	     *level);
	return rc ? rc : status;
}

/**
 * sc_get_sensor
 * @brief Set up the get-sensor rtas system call
 *
 * @param token
 * @param sensor
 * @param index
 * @param state
 * @return 0 on success, !0 otherwise
 */
int sc_get_sensor(int token, int sensor, int index, int *state)
{
	uint64_t elapsed = 0;
	__be32 be_state;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 2, 2, htobe32(sensor),
				  htobe32(index), &status, &be_state);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*state = be32toh(be_state);

	dbg1("(%d, %d, %p) = %d, %d\n", sensor, index, state, rc ? rc : status,
	     *state);
	return rc ? rc : status;
}

/**
 * sc_get_sysparm
 * @brief Setup the get-system-parameter rtas system call
 *
 * @param token
 * @param parameter
 * @param length
 * @param data
 * @return 0 on success, !0 otherwise
 */
int
sc_get_sysparm(int token, unsigned int parameter, unsigned int length,
	       char *data)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	do {
		rc = sc_rtas_call(token, 3, 1, htobe32(parameter),
				  htobe32(kernbuf_pa), htobe32(length),
				  &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(data, kernbuf, length);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	dbg1("(%d, %d, %p) = %d\n", parameter, length, data, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_get_time
 * @brief Set up the get-time rtas system call
 *
 * @param token
 * @param year
 * @param month
 * @param day
 * @param hour
 * @param min
 * @param sec
 * @param nsec
 * @return 0 on success, !0 otherwise
 */
int sc_get_time(int token, uint32_t *year, uint32_t *month, uint32_t *day, 
		uint32_t *hour, uint32_t *min, uint32_t *sec, uint32_t *nsec)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 0, 8, &status, year, month, day, hour,
				  min, sec, nsec);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*year = be32toh(*year);
	*month = be32toh(*month);
	*day = be32toh(*day);
	*hour = be32toh(*hour);
	*min = be32toh(*min);
	*sec = be32toh(*sec);
	*nsec = be32toh(*nsec);

	dbg1("() = %d, %d, %d, %d, %d, %d, %d, %d\n", rc ? rc : status, *year,
			*month, *day, *hour, *min, *sec, *nsec);
	return rc ? rc : status;
}

/**
 * sc_get_vpd
 *
 * @param token
 * @param loc_code
 * @param workarea
 * @param size
 * @param sequence
 * @param seq_next
 * @param bytes_ret
 * @return 0 on success, !0 otherwise
 */
int sc_get_vpd(int token, char *loc_code, char *workarea, size_t size, 
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
	int status;
	int rc;
	
	rc = sc_get_rmo_buffer(size + PAGE_SIZE, &rmobuf, &rmo_pa);
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
		rc = sc_rtas_call(token, 4, 3, htobe32(loc_pa),
				  htobe32(kernbuf_pa), htobe32(size),
				  sequence, &status, seq_next,
				  bytes_ret);
		if (rc < 0)
			break;
		
		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);
	
	if (rc == 0)
		memcpy(workarea, kernbuf, size);

	(void) sc_free_rmo_buffer(rmobuf, rmo_pa, size + PAGE_SIZE);
	
	*seq_next = be32toh(*seq_next);
	*bytes_ret = be32toh(*bytes_ret);

	dbg1("(%s, 0x%p, %d, %d) = %d, %d, %d", loc_code ? loc_code : "NULL",
	    	workarea, size, sequence, status, *seq_next, *bytes_ret);
	return rc ? rc : status;	
}

/**
 * sc_lpar_perftools
 *
 * @param token
 * @param subfunc
 * @param workarea
 * @param length
 * @param sequence
 * @param seq_next
 * @return 0 on success, !0 otherwise
 */
int sc_lpar_perftools(int token, int subfunc, char *workarea,
		      unsigned int length, unsigned int sequence,
		      unsigned int *seq_next)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, PAGE_SIZE);

	*seq_next = htobe32(sequence);
	do {
		sequence = *seq_next;
		rc = sc_rtas_call(token, 5, 2, htobe32(subfunc), 0,
				  htobe32(kernbuf_pa), htobe32(length),
				  sequence, &status, seq_next);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, length);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	*seq_next = be32toh(*seq_next);

	dbg1("(%d, %p, %d, %d, %p) = %d, %d\n", subfunc, workarea,
	     length, sequence, seq_next, rc ? rc : status, *seq_next);
	return rc ? rc : status;
}

/**
 * sc_platform_dump
 *
 * @param token
 * @param dump_tag
 * @param sequence
 * @param buffer
 * @param length
 * @param seq_next
 * @param bytes_ret
 * @return 0 on success, !0 otherwise
 */
int
sc_platform_dump(int token, uint64_t dump_tag, uint64_t sequence, void *buffer,
		 size_t length, uint64_t * seq_next, uint64_t * bytes_ret)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa = 0;
	uint32_t next_hi, next_lo;
	uint32_t bytes_hi, bytes_lo;
	void *kernbuf;
	int status;
	int rc;

	if (buffer) {
		rc = sc_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
		if (rc)
			return rc;
	}

	*seq_next = htobe64(sequence);
	do {
		sequence = *seq_next;
		rc = sc_rtas_call(token, 6, 5, BITS32_HI(htobe64(dump_tag)),
				  BITS32_LO(htobe64(dump_tag)),
				  BITS32_HI(sequence),
				  BITS32_LO(sequence),
				  htobe32(kernbuf_pa), htobe32(length),
				  &status, &next_hi, &next_lo, &bytes_hi,
				  &bytes_lo);
		if (rc < 0)
			break;
		
		*seq_next = BITS64(next_hi, next_lo);
		dbg1("%s: seq_next = 0x%llx\n", __FUNCTION__, *seq_next);

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (buffer && (rc == 0))
		memcpy(buffer, kernbuf, length);
	
	if (kernbuf)
		(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	next_hi = be32toh(next_hi);
	next_lo = be32toh(next_lo);
	bytes_hi = be32toh(bytes_hi);
	bytes_lo = be32toh(bytes_lo);

	*bytes_ret = BITS64(be32toh(bytes_hi), be32toh(bytes_lo));

	dbg1("(0x%llx, 0x%llx, %p, %d, %p, %p) = %d, 0x%llx, 0x%llx\n",
	     dump_tag, sequence, buffer, length, seq_next, bytes_ret,
	     rc ? rc : status, *seq_next, *bytes_ret);
	return rc ? rc : status;
}

/**
 * sc_read_slot_reset
 * @brief Set up the read-slot-reset-state rtas system call
 *
 * @param token
 * @param cfg_addr configuration address of slot to read
 * @param phbid PHB ID of slot to read
 * @param state
 * @param eeh
 * @return 0 on success, !0 otherwise
 */
int
sc_read_slot_reset(int token, uint32_t cfg_addr, uint64_t phbid, int *state,
		   int *eeh)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 3, 3, htobe32(cfg_addr),
				  BITS32_HI(htobe64(phbid)),
				  BITS32_LO(htobe64(phbid)), &status,
				  state, eeh);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*state = be32toh(*state);
	*eeh = be32toh(*eeh);

	dbg1("(0x%x, 0x%llx, %p, %p) = %d, %d, %d\n", cfg_addr, phbid, state,
	     eeh, rc ? rc : status, *state, *eeh);
	return rc ? rc : status;
}

/**
 * sc_scan_log_dump
 *
 * @param token
 * @param buffer
 * @param length
 * @return 0 on success, !0 otherwise
 */
int sc_scan_log_dump(int token, void *buffer, size_t length)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(length, &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, buffer, length);
	do {
		rc = sc_rtas_call(token, 2, 1, htobe32(kernbuf_pa),
				  htobe32(length), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(buffer, kernbuf, length);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, length);

	dbg1("(%p, %d) = %d\n", buffer, length, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_dynamic_indicator
 *
 * @param token
 * @param indicator
 * @param new_value
 * @param loc_code
 * @return 0 on success, !0 otherwise
 */
int sc_set_dynamic_indicator(int token, int indicator, int new_value,
			     void *loc_code)
{
	uint32_t loc_pa = 0;
	uint64_t elapsed = 0;
	void *locbuf;
	uint32_t size;
	int status;
	int rc;

	size = *(uint32_t *)loc_code + sizeof(uint32_t);

	rc = sc_get_rmo_buffer(size, &locbuf, &loc_pa);
	if (rc)
		return rc;

	memcpy(locbuf, loc_code, size);

	do {
		rc = sc_rtas_call(token, 3, 1, htobe32(indicator),
				  htobe32(new_value), htobe32(loc_pa), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);


	(void) sc_free_rmo_buffer(locbuf, loc_pa, size);

	dbg1("(%d, %d, %s) = %d\n", indicator, new_value, (char *)loc_code,
	     rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_eeh_option
 *
 * @param token
 * @param cfg_addr
 * @param phbid
 * @param function
 * @return 0 on success, !0 otherwise
 */
int
sc_set_eeh_option(int token, uint32_t cfg_addr, uint64_t phbid, int function)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 4, 1, htobe32(cfg_addr),
				  BITS32_HI(htobe64(phbid)),
				  BITS32_LO(htobe64(phbid)),
				  htobe32(function), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(0x%x, 0x%llx, %d) = %d\n", cfg_addr, phbid, function,
	     rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_indicator
 *
 * @param token
 * @param indicator
 * @param index
 * @param new_value
 * @return 0 on success, !0 otherwise
 */
int sc_set_indicator(int token, int indicator, int index, int new_value)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 3, 1, htobe32(indicator),
				  htobe32(index), htobe32(new_value), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(%d, %d, %d) = %d\n", indicator, index, new_value,
	     rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_power_level
 *
 * @param token
 * @param powerdomain
 * @param level
 * @param setlevel
 * @return 0 on success, !0 otherwise
 */
int sc_set_power_level(int token, int powerdomain, int level, int *setlevel)
{
	uint64_t elapsed = 0;
	__be32 be_setlevel;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 2, 2, htobe32(powerdomain),
				  htobe32(level), &status, &be_setlevel);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	*setlevel = be32toh(be_setlevel);

	dbg1("(%d, %d, %p) = %d, %d\n", powerdomain, level, setlevel,
	     rc ? rc : status, *setlevel);
	return rc ? rc : status;
}

/**
 * sc_set_poweron_time
 *
 * @param token
 * @param year
 * @param month
 * @param day
 * @param hour
 * @param min
 * @param sec
 * @param nsec
 * @return 0 on success, !0 otherwise
 */
int
sc_set_poweron_time(int token, uint32_t year, uint32_t month, uint32_t day,
		    uint32_t hour, uint32_t min, uint32_t sec, uint32_t nsec)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 7, 1, htobe32(year), htobe32(month),
				  htobe32(day), htobe32(hour), htobe32(min),
				  htobe32(sec), htobe32(nsec), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(%d, %d, %d, %d, %d, %d, %d) = %d\n", year, month, day, hour,
	     min, sec, nsec, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_sysparm
 *
 * @param token
 * @param parameter
 * @param data
 * @return 0 on success, !0 otherwise
 */
int sc_set_sysparm(int token, unsigned int parameter, char *data)
{
	uint64_t elapsed = 0;
	uint32_t kernbuf_pa;
	void *kernbuf;
	int status;
	short size;
	int rc;

	size = *(short *)data;
	rc = sc_get_rmo_buffer(size + sizeof(short), &kernbuf, &kernbuf_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, data, size + sizeof(short));

	do {
		rc = sc_rtas_call(token, 2, 1, htobe32(parameter),
				  htobe32(kernbuf_pa), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	(void)sc_free_rmo_buffer(kernbuf, kernbuf_pa, size + sizeof(short));

	dbg1("(%d, %p) = %d\n", parameter, data, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_set_time
 *
 * @param token
 * @param year
 * @param month
 * @param day
 * @param hour
 * @param min
 * @param sec
 * @param nsec
 * @return 0 on success, !0 otherwise
 */
int sc_set_time(int token, uint32_t year, uint32_t month, uint32_t day, 
		uint32_t hour, uint32_t min, uint32_t sec, uint32_t nsec)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 7, 1, htobe32(year), htobe32(month),
				htobe32(day), htobe32(hour), htobe32(min),
				htobe32(sec), htobe32(nsec), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("(%d, %d, %d, %d, %d, %d, %d) = %d\n", year, month, day, hour,
			min, sec, nsec, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_suspend_me
 *
 * @param token
 * @return 0 on success, !0 otherwise
 */
int sc_suspend_me(int token, uint64_t streamid)
{
	uint64_t elapsed = 0;
	int status;
	int rc;

	do {
		rc = sc_rtas_call(token, 2, 1, BITS32_HI(htobe64(streamid)),
				  BITS32_LO(htobe64(streamid)), &status);
		if (rc)
			return rc;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	dbg1("() = %d\n", rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_update_nodes
 * @brief Set up the ibm,update-nodes rtas system call
 *
 * @param token
 * @param workarea
 * @param scope
 * @return 0 on success, !0 otherwise
 *
 * Note that the PAPR defines the work area as 4096 bytes (as
 * opposed to a page), thus we use that rather than PAGE_SIZE below.
 */
int sc_update_nodes(int token, char *workarea, unsigned int scope)
{
	uint32_t workarea_pa;
	uint64_t elapsed = 0;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(4096, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, 4096);

	do {
		rc = sc_rtas_call(token, 2, 1, htobe32(workarea_pa),
				  htobe32(scope), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, 4096);

	(void)sc_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	dbg1("(%p) %d = %d\n", workarea, scope, rc ? rc : status);
	return rc ? rc : status;
}

/**
 * sc_update_properties
 * @brief Set up the ibm,update-properties rtas system call
 *
 * @param token
 * @param workarea
 * @param scope
 * @return 0 on success, !0 otherwise
 *
 * Note that the PAPR defines the work area as 4096 bytes (as
 * opposed to a page), thus we use that rather than PAGE_SIZE below.
 */
int sc_update_properties(int token, char *workarea, unsigned int scope)
{
	uint32_t workarea_pa;
	uint64_t elapsed = 0;
	void *kernbuf;
	int status;
	int rc;

	rc = sc_get_rmo_buffer(4096, &kernbuf, &workarea_pa);
	if (rc)
		return rc;

	memcpy(kernbuf, workarea, 4096);

	do {
		rc = sc_rtas_call(token, 2, 1, htobe32(workarea_pa),
				  htobe32(scope), &status);
		if (rc < 0)
			break;

		rc = handle_delay(status, &elapsed);
	} while (rc == CALL_AGAIN);

	if (rc == 0)
		memcpy(workarea, kernbuf, 4096);

	(void)sc_free_rmo_buffer(kernbuf, workarea_pa, PAGE_SIZE);

	dbg1("(%p) %d = %d\n", workarea, scope, rc ? rc : status);
	return rc ? rc : status;
}
/* ========================= END OF FILE ================== */
