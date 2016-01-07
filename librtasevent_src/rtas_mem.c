/**
 * @file rtas_mem.c
 * @brief RTAS Memory Controller detected routines
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
 * @author Jake Moilanen <moilanen@austin.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * parse_mem_scn
 *
 */
int
parse_mem_scn(struct rtas_event *re)
{
    struct rtas_mem_scn *mem;

    mem = malloc(sizeof(*mem));
    if (mem == NULL) {
        errno = ENOMEM;
        return -1;
    }

    mem->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(mem), re, RE_V4_SCN_SZ);
    add_re_scn(re, mem, RTAS_MEM_SCN);

    return 0;
}

/**
 * rtas_get_mem_scn
 * @brief Retrieve the Memory Detected failure section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for memory section
 */
struct rtas_mem_scn *
rtas_get_mem_scn(struct rtas_event *re)
{
    return (struct rtas_mem_scn *)get_re_scn(re, RTAS_MEM_SCN);
}

/**
 * print_re_mem_scn
 * @brief print the contents of a RTAS memory controller detected error section
 *
 * @param res rtas_event_scn pointer for memory section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_mem_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_mem_scn *mem = (struct rtas_mem_scn *)shdr;
    int len = 0;

    if (shdr->scn_id != RTAS_MEM_SCN) {
        errno = EFAULT;
        return 0;
    }
    
    len += print_scn_title("Memory Section");

    if (mem->uncorrectable) 
	len += rtas_print("Uncorrectable Memory error.\n");
    if (mem->ECC) 
	len += rtas_print("ECC Correctable error.\n");
    if (mem->threshold_exceeded) 
	len += rtas_print("Correctable threshold exceeded.\n");
    if (mem->control_internal) 
	len += rtas_print("Memory Controller internal error.\n");
    
    if (mem->bad_address) 
	len += rtas_print("Memory Address error.\n");
    if (mem->bad_data) 
	len += rtas_print("Memory Data error.\n");
    if (mem->bus)
	len += rtas_print("Memory bus/switch internal error.\n");
    if (mem->timeout) 
	len += rtas_print("Memory timeout.\n");
    
    if (mem->sysbus_parity) 
	len += rtas_print("System bus parity.\n");
    if (mem->sysbus_timeout) 
	len += rtas_print("System bus timeout.\n");
    if (mem->sysbus_protocol) 
	len += rtas_print("System bus protocol/transfer.\n");
    if (mem->hostbridge_timeout) 
	len += rtas_print("I/O Host Bridge timeout.\n");
    if (mem->hostbridge_parity) 
	len += rtas_print("I/O Host Bridge parity.\n");

    if (shdr->re->version >= 3) {
        if (mem->support) 
	    len += rtas_print("System support function error.\n");
        if (mem->sysbus_internal) 
	    len += rtas_print("System bus internal hardware/switch error.\n");
    }
    
    len += rtas_print("Memory Controller that detected failure: %x.\n", 
	              mem->controller_detected);
    len += rtas_print("Memory Controller that faulted: %x.\n", 
	              mem->controller_faulted);
    
    len += rtas_print(PRNT_FMT_ADDR, "Failing address:", 
	              mem->failing_address_hi, mem->failing_address_lo);
    
    len += rtas_print(PRNT_FMT_2, "ECC syndrome bits:", mem->ecc_syndrome,
                      "Memory Card:", mem->memory_card);
    len += rtas_print(PRNT_FMT_2, "Failing element:", mem->element, 
                      "Sub element bits:", mem->sub_elements);

    len += rtas_print("\n");
    return len;
}
