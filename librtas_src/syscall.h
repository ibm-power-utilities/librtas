/**
 * @file syscall.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>

/* Syscall number */
#ifndef __NR_rtas
#define __NR_rtas 255
#endif

#define MAX_ARGS 16

typedef uint32_t rtas_arg_t;

/* Based on include/asm-ppc64/rtas.h */
struct rtas_args {
	uint32_t token;
	uint32_t ninputs;
	uint32_t nret; 
	rtas_arg_t args[MAX_ARGS];
	rtas_arg_t *rets;     /* Pointer to return values in args[]. */
};  

struct region {
	uint64_t addr;
	uint32_t size;
	struct region *next;
};

extern int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t *phys_addr);
extern int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size);
extern int interface_exists();
extern int read_entire_file(int fd, char **buf, size_t *len);
extern int rtas_token(const char *call_name);

#define BITS32_LO(_num) (uint32_t) (_num & 0xffffffffll)
#define BITS32_HI(_num) (uint32_t) (_num >> 32) 
#define BITS64(_high, _low) (uint64_t) (((uint64_t) _high << 32) | _low)

extern int dbg_lvl;

#define dbg(_fmt, _args...)						  \
	do {								  \
		if (dbg_lvl >= 0)					  \
			printf("librtas %s(): " _fmt, __func__, ##_args); \
	} while (0)

#endif
