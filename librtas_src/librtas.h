/**
 * @file librtas.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#ifndef _LIBRTAS_H_
#define _LIBRTAS_H_

#include <sys/types.h>
#include <stdint.h>

#define RTAS_KERNEL_INT -1001	/* No Kernel Interface to Firmware */
#define RTAS_KERNEL_IMP -1002	/* No Kernel Implementation of Function */
#define RTAS_PERM	-1003   /* Non-root caller */
#define RTAS_NO_MEM 	-1004   /* Out of heap memory */
#define RTAS_NO_LOWMEM	-1005	/* Kernel out of low memory */
#define RTAS_FREE_ERR	-1006	/* Attempt to free nonexistant rmo buffer */
#define RTAS_TIMEOUT	-1007	/* RTAS delay exceeded specified timeout */
#define RTAS_IO_ASSERT	-1098	/* Unexpected I/O Error */
#define RTAS_UNKNOWN_OP -1099	/* No Firmware Implementation of Function */

/* Adding a new RTAS call requires the following:
 * 1) A function prototype in librtas.h (this file) that roughly matches
 *    the RTAS call name.
 * 2) An implementation of the new function in common.c.  Use the existing
 *    ones as an example.
 * 3) A function pointer definition in struct rtas_operations in common.h.
 *    Prepend a "token" argument to the argument list.
 * 4) An implementation-specific version of the RTAS function, in 
 *    syscall_calls.c and/or procfs_calls.c.  Remember to add your new 
 *    function to the rtas_operations struct at the top of the file.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int rtas_activate_firmware();
extern int rtas_cfg_connector(char *workarea);
extern int rtas_delay_timeout(uint64_t timeout_ms);
extern int rtas_display_char(char c);
extern int rtas_display_msg(char *buf);
extern int rtas_errinjct(int etoken, int otoken, char *workarea);
extern int rtas_errinjct_close(int otoken);
extern int rtas_errinjct_open(int *otoken);
extern int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size);
extern int rtas_get_config_addr_info2(uint32_t cfg_addr, uint64_t phb_id, uint32_t func, uint32_t *info);
extern int rtas_get_dynamic_sensor(int sensor, void *loc_code, int *state);
extern int rtas_get_indices(int is_sensor, int type, char *workarea,
				size_t size, int start, int *next);
extern int rtas_get_power_level(int powerdomain, int *level);
extern int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t *phys_addr);
extern int rtas_get_sensor(int sensor, int index, int *state);
extern int rtas_get_sysparm(unsigned int parameter, unsigned int length,
				char *data);
extern int rtas_get_time(uint32_t *year, uint32_t *month, uint32_t *day,
				uint32_t *hour, uint32_t *min, uint32_t *sec, 
				uint32_t *nsec);
extern int rtas_get_vpd(char *loc_code, char *workarea, size_t size, 
				unsigned int sequence, unsigned int *seq_next,
				unsigned int *bytes_ret);
extern int rtas_lpar_perftools(int subfunc, char *workarea, 
				unsigned int length, unsigned int sequence, 
				unsigned int *seq_next);
extern int rtas_platform_dump(uint64_t dump_tag, uint64_t sequence, 
				void *buffer, size_t length, 
				uint64_t *next_seq, uint64_t *bytes_ret);
extern int rtas_read_slot_reset(uint32_t cfg_addr, uint64_t phbid, int *state, 
				int *eeh);
extern int rtas_scan_log_dump(void *buffer, size_t length);
extern int rtas_set_debug(int level);
extern int rtas_set_dynamic_indicator(int indicator, int new_value,
				void *loc_code);
extern int rtas_set_eeh_option(uint32_t cfg_addr, uint64_t phbid, 
				int function);
extern int rtas_set_indicator(int indicator, int index, int new_value);
extern int rtas_set_power_level(int powerdomain, int level, int *setlevel);
extern int rtas_set_poweron_time(uint32_t year, uint32_t month, uint32_t day,
				uint32_t hour, uint32_t min, uint32_t sec, 
				uint32_t nsec);
extern int rtas_set_sysparm(unsigned int parameter, char *data);
extern int rtas_set_time(uint32_t year, uint32_t month, uint32_t day, 
				uint32_t hour, uint32_t min, uint32_t sec, 
				uint32_t nsec);
extern int rtas_suspend_me(uint64_t streamid);
extern int rtas_update_nodes(char *workarea, unsigned int scope);
extern int rtas_update_properties(char *workarea, unsigned int scope);

#ifdef __cplusplus
}
#endif
	
#endif /* _LIBRTAS_H_ */
