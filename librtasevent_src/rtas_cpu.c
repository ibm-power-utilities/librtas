/**
 * @file rtas_cpu.c
 * @brief RTAS error log detail for CPU-detected failures
 *
 * This provides the definition of RTAS CPU-detected error log
 * format and a print routine to dump the contents of a 
 * cpu-detected error log.
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
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
 * parse_cpu_scn
 *
 */
int
parse_cpu_scn(struct rtas_event *re)
{
    struct rtas_cpu_scn *cpu_scn;

    cpu_scn = malloc(sizeof(*cpu_scn));
    if (cpu_scn == NULL) {
        errno = ENOMEM;
        return -1;
    }

    cpu_scn->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(cpu_scn), re, RE_V4_SCN_SZ);
    add_re_scn(re, cpu_scn, RTAS_CPU_SCN);

    return 0;
}

/**
 * rtas_get_cpu_scn
 * @brief Retrieve the CPU section of the RTAS Event
 * 
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for cpu section
 */
struct rtas_cpu_scn *
rtas_get_cpu_scn(struct rtas_event *re)
{
    return (struct rtas_cpu_scn *)get_re_scn(re, RTAS_CPU_SCN);
}


/**
 * print_cpu_failure
 * @brief Print the contents of a cpu section
 *
 * @param res rtas_event_scn pointer to cpu section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_cpu_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_cpu_scn *cpu;
    int len = 0;

    if (shdr->scn_id != RTAS_CPU_SCN) {
        errno = EFAULT;
        return 0;
    }

    cpu = (struct rtas_cpu_scn *)shdr;

    len += print_scn_title("CPU Section");

    if (cpu->internal) 
       len += rtas_print("Internal error (not cache).\n");
    if (cpu->intcache) 
       len += rtas_print("Internal cache.\n");
    if (cpu->extcache_parity) 
       len += rtas_print("External cache parity (or multi-bit).\n");
    if (cpu->extcache_ecc) 
       len += rtas_print("External cache ECC.\n");
    if (cpu->sysbus_timeout) 
       len += rtas_print("System bus timeout.\n");
    if (cpu->io_timeout) 
       len += rtas_print("I/O timeout.\n");
    if (cpu->sysbus_parity) 
       len += rtas_print("System bus parity.\n");
    if (cpu->sysbus_protocol) 
       len += rtas_print("System bus protocol/transfer.\n");

    len += rtas_print(PRNT_FMT_2, "CPU id:", cpu->id,
                      "Failing Element:", cpu->element);
    
    len += rtas_print(PRNT_FMT_ADDR, "Failing address:", 
	              cpu->failing_address_hi, 
                      cpu->failing_address_lo);
    
    if ((shdr->re->version >= 4) && (cpu->try_reboot))
	len += rtas_print("A reboot of the system may correct the problem.\n");

    len += rtas_print("\n");
    return len;
}
