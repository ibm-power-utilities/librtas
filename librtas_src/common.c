/**
 * @file common.c
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "librtas.h"
#include "common.h"

extern struct rtas_operations syscall_rtas_ops;
extern struct rtas_operations procfs_rtas_ops;

struct librtas_config config = { NULL, 0ll, 0 };

#define SANITY_CHECKS(_name)					\
	/* Check credentials */					\
	if (geteuid() != (uid_t) 0)				\
		return RTAS_PERM;				\
								\
	/* Check for any kernel RTAS interface */		\
	if (config.rtas_ops == NULL)				\
		if (init_interface()) 				\
			return RTAS_KERNEL_INT;			\
								\
	/* Check for kernel implementation of function */	\
	if (config.rtas_ops->_name == NULL) 			\
		return RTAS_KERNEL_IMP

#define CALL_RTAS_METHOD(_name, _token, _args...)		\
	SANITY_CHECKS(_name);					\
								\
	/* Call interface-specific RTAS routine,		\
	 * passing in token */					\
	return config.rtas_ops->_name(_token, ##_args)

#define CALL_HELPER_METHOD(_name, _args...)			\
	SANITY_CHECKS(_name);					\
								\
	/* Call interface-specific helper routine */		\
	return config.rtas_ops->_name(_args)			\

#define MAX_PATH_LEN 80

/**
 * open_proc_rtas_file
 * @brief Open the proc rtas file
 *
 * @param name filename to open 
 * @param mode mode to open file in
 * @return results of open() call
 */
int open_proc_rtas_file(const char *name, int mode)
{
	const char *proc_rtas_paths[] = { "/proc/ppc64/rtas", "/proc/rtas" };
	char full_name[MAX_PATH_LEN];
	int npaths;
	int fd;
	int i;

	npaths = sizeof(proc_rtas_paths) / sizeof(char *);
	for (i = 0; i < npaths; i++) {
		sprintf(full_name, "%s/%s", proc_rtas_paths[i], name);
		fd = open(full_name, mode, S_IRUSR | S_IWUSR);
		if (fd >= 0)
			break;
	}

	if (fd < 0)
		dbg1("Failed to open %s\n", full_name);

	return fd;
}

/**
 * init_interface
 * @brief Initialize the librtas interface to use
 *
 * @return 0 on sucess, 1 on failure
 */
static int init_interface()
{
	if (syscall_rtas_ops.interface_exists())
		config.rtas_ops = &syscall_rtas_ops;
	else if (procfs_rtas_ops.interface_exists())
		config.rtas_ops = &procfs_rtas_ops;
	else
		return 1;

	return 0;
}

/**
 * rtas_activate_firmware
 * @brief Interface for ibm,activate-firmware rtas call
 *
 * @return 0 on success, !0 on failure
 */ 
int rtas_activate_firmware()
{
	int token = rtas_token("ibm,activate-firmware");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(activate_firmware, token);
}

/**
 * rtas_cfg_connector
 * @brief Interface for ibm,configure-connector rtas call
 *
 * @param workarea buffer containg args to ibm,configure-connector
 * @return 0 on success, !0 on failure
 */
int rtas_cfg_connector(char *workarea)
{
	int token = rtas_token("ibm,configure-connector");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(cfg_connector, token, workarea);
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
	CALL_HELPER_METHOD(delay_timeout, timeout_ms);
}

/**
 * rtas_diaply_char
 * @brief Interface for display-character rtas call
 *
 * @param c character to display
 * @return 0 on success, !0 otherwise
 */
int rtas_display_char(char c)
{
	int token = rtas_token("display-character");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(display_char, token, c);
}

/**
 * rtas_diaplay_msg
 * @brief Interface for ibm,display-message rtas call
 *
 * @param buf message to display
 * @return 0 on success, !0 otherwise
 */
int rtas_display_msg(char *buf)
{
	int token = rtas_token("ibm,display-message");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(display_msg, token, buf);
}

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
	int token = rtas_token("ibm,errinjct");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(errinjct, token, etoken, otoken, workarea);
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
	int token = rtas_token("ibm,close-errinjct");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(errinjct_close, token, otoken);
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
	int token = rtas_token("ibm,open-errinjct");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(errinjct_open, token, otoken);
}

/**
 * rtas_free_rmp_buffer
 * @brief free the rmo buffer used by librtas
 *
 * @param buf rmo buffer to free
 * @param phys_addr physical address of the buffer
 * @param size buf size
 * @return 0 on success, !0 otherwise
 */
int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size)
{
	CALL_HELPER_METHOD(free_rmo_buffer, buf, phys_addr, size);
}

/**
 * rtas_get_config_addr_info2
 * @brief Interface to ibm,get-config-addr-info2 rtas call
 *
 * On successful completion the info value is returned.
 *
 * @param config_addr
 * @param PHB_Unit_ID
 * @param function
 * @param info
 * @return 0 on success, !0 otherwise
 */
int rtas_get_config_addr_info2(uint32_t config_addr,
    uint64_t phb_id, uint32_t function, uint32_t *info)
{
	int token = rtas_token("ibm,get-config-addr-info2");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_config_addr_info2, token, config_addr,
	   phb_id, function, info);
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
	int token = rtas_token("ibm,get-dynamic-sensor-state");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_dynamic_sensor, token, sensor, loc_code, state);
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
	int token = rtas_token("ibm,get-indices");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_indices, token, is_sensor, type, workarea, size,
			 start, next);
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
	int token = rtas_token("get-power-level");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_power_level, token, powerdomain, level);
}

/**
 * rtas_get_rmo_buffer
 * @brief Retrive the RMO buffer used by librtas
 *
 * On successful completion the buf parameter will reference an allocated
 * area of RMO memory and the phys_addr parameter will refernce the
 * physical address of the RMO buffer.
 * 
 * @param size buffer size to retrieve
 * @param buf reference to buffer pointer
 * @param phys_addr reference to the phys_addr variable
 * @return 0 on success, !0 otherwise
 */
int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t * phys_addr)
{
	CALL_HELPER_METHOD(get_rmo_buffer, size, buf, phys_addr);
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
	int token = rtas_token("get-sensor-state");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_sensor, token, sensor, index, state);
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
	int token = rtas_token("ibm,get-system-parameter");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_sysparm, token, parameter, length, data);
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
				uint32_t *hour, uint32_t *min, uint32_t *sec, 
				uint32_t *nsec)
{
	int token = rtas_token("get-time-of-day");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_time, token, year, month, day, hour, min, sec, 
			nsec);
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
	int token = rtas_token("ibm,get-vpd");
	
	if (token < 0)
		return token;

	CALL_RTAS_METHOD(get_vpd, token, loc_code, workarea, size, sequence,
				seq_next, bytes_ret);
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
	int token = rtas_token("ibm,lpar-perftools");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(lpar_perftools, token, subfunc, workarea, length,
			 sequence, seq_next);
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
int rtas_platform_dump(uint64_t dump_tag, uint64_t sequence,
		       void *buffer, size_t length,
		       uint64_t * next_seq, uint64_t * bytes_ret)
{
	int token = rtas_token("ibm,platform-dump");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(platform_dump, token, dump_tag, sequence, buffer,
			 length, next_seq, bytes_ret);
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
	int token = rtas_token("ibm,read-slot-reset-state");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(read_slot_reset, token, cfg_addr, phbid, state, eeh);
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
	int token = rtas_token("ibm,scan-log-dump");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(scan_log_dump, token, buffer, length);
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
	config.debug = level;

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
	int token = rtas_token("ibm,set-dynamic-indicator");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_dynamic_indicator, token, indicator, new_value,
			 loc_code);
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
	int token = rtas_token("ibm,set-eeh-option");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_eeh_option, token, cfg_addr, phbid, function);
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
	int token = rtas_token("set-indicator");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_indicator, token, indicator, index, new_value);
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
	int token = rtas_token("set-power-level");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_power_level, token, powerdomain, level, setlevel);
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
	int token = rtas_token("set-time-for-power-on");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_poweron_time, token, year, month, day, hour, min,
			 sec, nsec);
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
	int token = rtas_token("ibm,set-system-parameter");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_sysparm, token, parameter, data);
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
	int token = rtas_token("set-time-of-day");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(set_time, token, year, month, day, hour, min, sec, 
			nsec);
}

/**
 * rtas_suspend_me
 * @brief Interface for ibm,suspend-me rtas call
 *
 * @return 0 on success, !0 on failure
 */ 
int rtas_suspend_me(uint64_t streamid)
{
	int token = rtas_token("ibm,suspend-me");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(suspend_me, token, streamid);
}

/**
 * rtas_update_nodes
 * @brief Interface for ibm,update-nodes rtas call
 *
 * @param workarea input output work area for the rtas call
 * @param scope of call
 * @return 0 on success, !0 on failure
 */ 
int rtas_update_nodes(char *workarea, unsigned int scope )
{
	int token = rtas_token("ibm,update-nodes");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(update_nodes, token, workarea, scope);
}

/**
 * rtas_update_properties
 * @brief Interface for ibm,update-properties rtas call
 *
 * @param workarea input output work area for the rtas call
 * @param scope of call
 * @return 0 on success, !0 on failure
 */ 
int rtas_update_properties(char *workarea, unsigned int scope )
{
	int token = rtas_token("ibm,update-properties");

	if (token < 0)
		return token;

	CALL_RTAS_METHOD(update_properties, token, workarea, scope);
}
