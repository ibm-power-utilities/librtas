/**
 * @file common.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdint.h>

#define RC_BUSY -2
#define EXTENDED_DELAY_MIN 9900
#define EXTENDED_DELAY_MAX 9905

#define PAGE_SIZE 4096

/* 
 * When adding an RTAS call, argument list below should 
 * consist of an int token followed by the call arguments 
 * defined in librtas.h
 */
struct rtas_operations {
	int (*activate_firmware)(int token);
	int (*cfg_connector)(int token, char *workarea);
	int (*delay_timeout)(uint64_t timeout_ms);
	int (*display_char)(int token, char c);
	int (*display_msg)(int token, char *buf);
	int (*errinjct)(int token, int etoken, int otoken, char *workarea);
	int (*errinjct_close)(int token, int otoken);
	int (*errinjct_open)(int token, int *otoken);
	int (*free_rmo_buffer)(void *buf, uint32_t phys_addr, size_t size);
	int (*get_config_addr_info2)(int token, uint32_t cfg_addr, 
	         uint64_t phb_id, uint32_t func, uint32_t *info);
	int (*get_dynamic_sensor)(int token, int sensor, void *loc_code,
				int *state);
	int (*get_indices)(int token, int is_sensor, int type, char *workarea, 
				size_t size, int start, int *next);
	int (*get_power_level)(int token, int powerdomain, int *level);
	int (*get_rmo_buffer)(size_t size, void **buf, uint32_t *phys_addr);
	int (*get_sensor)(int token, int sensor, int index, int *state);
	int (*get_sysparm)(int token, unsigned int parameter, 
				unsigned int length, char *data);
	int (*get_time)(int token, uint32_t *year, uint32_t *month, 
				uint32_t *day, uint32_t *hour, uint32_t *min, 
				uint32_t *sec, uint32_t *nsec);
	int (*get_vpd)(int token, char *loc_code, char *workarea, size_t size, 
				unsigned int sequence, unsigned int *seq_next,
				unsigned int *bytes_ret);
	int (*lpar_perftools)(int token, int subfunc, char *workarea, 
				unsigned int length, unsigned int sequence, 
				unsigned int *seq_next);
	int (*platform_dump)(int token, uint64_t dump_tag, uint64_t sequence, 
				void *buffer, size_t length, 
				uint64_t *next_seq, uint64_t *bytes_ret);
	int (*read_slot_reset)(int token, uint32_t cfg_addr, uint64_t phbid,
				int *state, int *eeh);
	int (*scan_log_dump)(int token, void *buffer, size_t length);
	int (*set_dynamic_indicator)(int token, int indicator, int new_value, 
				void *loc_code);
	int (*set_eeh_option)(int token, uint32_t cfg_addr, uint64_t phbid, 
				int function);
	int (*set_indicator)(int token, int indicator, int index, 
				int new_value);
	int (*set_power_level)(int token, int powerdomain, int level, 
				int *setlevel);
	int (*set_poweron_time)(int token, uint32_t year, uint32_t month, 
				uint32_t day, uint32_t hour, uint32_t min, 
				uint32_t sec, uint32_t nsec);
	int (*set_sysparm)(int token, unsigned int parameter, char *data);
	int (*set_time)(int token, uint32_t year, uint32_t month, uint32_t day, 
				uint32_t hour, uint32_t min, uint32_t sec, 
				uint32_t nsec);
	int (*suspend_me)(int token, uint64_t streamid);
	int (*update_nodes)(int token, char *workarea, unsigned int scope);
	int (*update_properties)(int token, char *workarea, unsigned int scope);
	int (*interface_exists)();
};

struct librtas_config {
	struct rtas_operations *rtas_ops;
	uint64_t rtas_timeout_ms;
	int debug;
};

extern int open_proc_rtas_file(const char *name, int mode);
extern int rtas_token(const char *call_name);
extern int read_entire_file(int fd, char **buf, size_t *len);

extern struct librtas_config config;

#define dbg(_lvl, _fmt, _args...) 					\
	do {								\
		if (config.debug >= _lvl) 				\
			printf("librtas %s(): " _fmt, __FUNCTION__, ##_args); \
	} while (0)

#define dbg1(_fmt, _args...)						\
	do {								\
		dbg(1, _fmt, ##_args);					\
	} while (0)							\

#endif
