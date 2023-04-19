/**
 * @file internal.h
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

#ifndef LIBRTAS_INTERNAL_H
#define LIBRTAS_INTERNAL_H

#include <stdint.h>

#define WORK_AREA_SIZE 4096
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

int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t *phys_addr);
int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size);
int interface_exists();
int read_entire_file(int fd, char **buf, size_t *len);
int rtas_token(const char *call_name);
int sanity_check(void);

#define BITS32_LO(_num) (uint32_t) (_num & 0xffffffffll)
#define BITS32_HI(_num) (uint32_t) (_num >> 32) 
#define BITS64(_high, _low) (uint64_t) (((uint64_t) _high << 32) | _low)

extern int dbg_lvl;

#define dbg(_fmt, _args...)						  \
	do {								  \
		if (dbg_lvl > 0)					  \
			printf("librtas %s(): " _fmt, __func__, ##_args); \
	} while (0)

#endif /* LIBRTAS_INTERNAL_H */
