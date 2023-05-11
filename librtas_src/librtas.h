/**
 * @file librtas.h
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

#ifndef LIBRTAS_H
#define LIBRTAS_H

#include <stdio.h>
#include <unistd.h>
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

#define RC_BUSY -2
#define EXTENDED_DELAY_MIN 9900
#define EXTENDED_DELAY_MAX 9905

/* Adding a new RTAS call requires the following:
 * 1) A function prototype in librtas.h (this file) that roughly matches
 *    the RTAS call name.
 * 2) An implementation of the RTAS function, in syscall_calls.c.
 */

#ifdef __cplusplus
extern "C" {
#endif

int rtas_activate_firmware(void);
int rtas_cfg_connector(char *workarea);
int rtas_delay_timeout(uint64_t timeout_ms);
int rtas_display_char(char c);
int rtas_display_msg(char *buf);
int rtas_errinjct(int etoken, int otoken, char *workarea);
int rtas_errinjct_close(int otoken);
int rtas_errinjct_open(int *otoken);
int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size);
int rtas_get_config_addr_info2(uint32_t cfg_addr, uint64_t phb_id,
			       uint32_t func, uint32_t *info);
int rtas_get_dynamic_sensor(int sensor, void *loc_code, int *state);
int rtas_get_indices(int is_sensor, int type, char *workarea,
		     size_t size, int start, int *next);
int rtas_get_power_level(int powerdomain, int *level);
int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t *phys_addr);
int rtas_get_sensor(int sensor, int index, int *state);
int rtas_get_sysparm(unsigned int parameter, unsigned int length, char *data);
int rtas_get_time(uint32_t *year, uint32_t *month, uint32_t *day,
		  uint32_t *hour, uint32_t *min, uint32_t *sec, uint32_t *nsec);
int rtas_get_vpd(char *loc_code, char *workarea, size_t size,
		 unsigned int sequence, unsigned int *seq_next,
		 unsigned int *bytes_ret);
int rtas_lpar_perftools(int subfunc, char *workarea,
			unsigned int length, unsigned int sequence,
			unsigned int *seq_next);
int rtas_platform_dump(uint64_t dump_tag, uint64_t sequence,
		       void *buffer, size_t length,
		       uint64_t *next_seq, uint64_t *bytes_ret);
int rtas_read_slot_reset(uint32_t cfg_addr, uint64_t phbid, int *state, int *eeh);
int rtas_scan_log_dump(void *buffer, size_t length);
int rtas_set_debug(int level);
int rtas_set_dynamic_indicator(int indicator, int new_value, void *loc_code);
int rtas_set_eeh_option(uint32_t cfg_addr, uint64_t phbid, int function);
int rtas_set_indicator(int indicator, int index, int new_value);
int rtas_set_power_level(int powerdomain, int level, int *setlevel);
int rtas_set_poweron_time(uint32_t year, uint32_t month, uint32_t day,
			  uint32_t hour, uint32_t min, uint32_t sec, uint32_t nsec);
int rtas_set_sysparm(unsigned int parameter, char *data);
int rtas_set_time(uint32_t year, uint32_t month, uint32_t day,
		  uint32_t hour, uint32_t min, uint32_t sec, uint32_t nsec);
int rtas_suspend_me(uint64_t streamid);
int rtas_update_nodes(char *workarea, unsigned int scope);
int rtas_update_properties(char *workarea, unsigned int scope);
int rtas_physical_attestation(char *workarea, int seq_num,
			      int *next_seq_num, int *work_area_bytes);

#ifdef __cplusplus
}
#endif

#endif /* LIBRTAS_H */
