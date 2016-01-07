/**
 * @file rtas_lri.c
 * @brief RTAS Logical Resource Identification (LRI) Data routines
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
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * parse_lri_scn
 *
 */
int
parse_lri_scn(struct rtas_event *re)
{
    struct rtas_lri_scn *lri;
    struct rtas_lri_scn_raw *rawhdr;
    
    lri = malloc(sizeof(*lri));
    if (lri == NULL) {
        errno = ENOMEM;
        return -1;
    }

    lri->shdr.raw_offset = re->offset;

    rawhdr = (struct rtas_lri_scn_raw *)(re->buffer + re->offset);

    parse_v6_hdr(&lri->v6hdr, &rawhdr->v6hdr);
    lri->resource = rawhdr->resource;
    lri->capacity = be16toh(rawhdr->capacity);

    lri->lri_mem_addr_lo = be32toh(rawhdr->lri_mem_addr_lo);
    lri->lri_mem_addr_hi = be32toh(rawhdr->lri_mem_addr_hi);

    re->offset += RE_LRI_SCN_SZ;
    add_re_scn(re, lri, RTAS_LRI_SCN);

    return 0;
}

/**
 * rtas_get_lri_scn
 * @brief Retrieve the Logical Resource ID (LRI) section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for lri section
 */
struct rtas_lri_scn *
rtas_get_lri_scn(struct rtas_event *re)
{
    return (struct rtas_lri_scn *)get_re_scn(re, RTAS_LRI_SCN);
}

/**
 * print_re_lriscn
 * @brief print the contents of a LRI section
 *
 * @param res rtas_event_scn pointer for lri section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_lri_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_lri_scn *lri;
    int len = 0;

    if (shdr->scn_id != RTAS_LRI_SCN) {
        errno = EFAULT;
        return 0;
    }

    lri = (struct rtas_lri_scn *)shdr;

    len += print_v6_hdr("Logical Resource Identification", &lri->v6hdr, 
                        verbosity);
    
    len += rtas_print(PRNT_FMT" ", "Resource Type:", lri->resource);
    switch (lri->resource) {
        case 0x10:
	    len += rtas_print("(Processor)\n" PRNT_FMT_R, "CPU ID:", 
                              lri->lri_cpu_id);
	    break;
            
        case 0x11:
	    len += rtas_print("(Shared Processor)\n" PRNT_FMT_R, 
                              "Entitled Capacity:", lri->capacity);
	    break;

        case 0x40: 
            len += rtas_print("(Memory Page)\n" PRNT_FMT_ADDR, 
                              "Logical Address:", lri->lri_mem_addr_hi,
                              lri->lri_mem_addr_lo);
	    break;

        case 0x41:
	    len += rtas_print("(Memory LMB)\n" PRNT_FMT_R, 
                              "DRC Index:", lri->lri_drc_index);
	    break;

	default:
	    len += rtas_print("(Unknown Resource)\n");
	    break;
    }
    
    len += rtas_print("\n");
    return len;
}
