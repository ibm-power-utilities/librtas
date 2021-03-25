/** 
 * @file rtas_vend.c
 * @brief RTAS error log detial for vendor specific extensions
 *
 * Copyright (C) IBM Corporation
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
 * parse_ibm diag_scn
 *
 */
int
parse_ibm_diag_scn(struct rtas_event *re)
{
    struct rtas_ibm_diag_scn *ibmdiag;

    ibmdiag = malloc(sizeof(*ibmdiag));
    if (ibmdiag == NULL) {
        errno = ENOMEM;
        return -1;
    }

    ibmdiag->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(ibmdiag), re, sizeof(uint32_t));
    add_re_scn(re, ibmdiag, RTAS_IBM_DIAG_SCN);

    return 0;
}

/**
 * rtas_get_ibm_diag_scn
 * @brief Retrieve the IBM Diagnostic Log section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for diagnostics log section
 */
struct rtas_ibm_diag_scn *
rtas_get_ibm_diag_scn(struct rtas_event *re)
{
    return (struct rtas_ibm_diag_scn *)get_re_scn(re, RTAS_IBM_DIAG_SCN);
}

/**
 * print_re_ibm_diag_scn
 * @brief print the contents of an IBM diagnostics log section
 *
 * @param res rtas_event_scn pointer for IBM diagnostics log section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_ibm_diag_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_ibm_diag_scn *ibmdiag;
    int len = 0;
   
    if (shdr->scn_id != RTAS_IBM_DIAG_SCN) {
        errno = EFAULT;
        return -1;
    }

    ibmdiag = (struct rtas_ibm_diag_scn *)shdr;

    len += print_scn_title("IBM Diagnostics Section");
    len += rtas_print(PRNT_FMT"\n", "Event ID:", ibmdiag->event_id);

    return len;
}

/**
 * parse_vend_specific_scn
 *
 */
int
parse_vend_errlog_scn(struct rtas_event *re)
{
    struct rtas_vend_errlog *ve;

    ve = malloc(sizeof(*ve));
    if (ve == NULL) {
        errno = ENOMEM;
        return -1;
    }

    ve->shdr.raw_offset = re->offset;
    rtas_copy(RE_SHDR_OFFSET(ve), re, 4);

    /* See if there is additional data */
    ve->vendor_data_sz = re->event_length - re->offset;
    if (ve->vendor_data_sz > 0) {
        ve->vendor_data = malloc(ve->vendor_data_sz);
        if (ve->vendor_data == NULL) {
            errno = ENOMEM;
            free(ve);
            return -1;
        }

        rtas_copy(ve->vendor_data, re, ve->vendor_data_sz);
    }
    
    add_re_scn(re, ve, RTAS_VEND_ERRLOG_SCN);
    return 0;
}

/**
 * rtas_get_vend_specific
 * @brief retrive a vendor specific section of the RTAS event
 *
 * @param re parsed rtas event
 * @return reference to a rtas_event_scn on success, NULL on failure
 */
struct rtas_vend_errlog_scn *
rtas_get_vend_errlog_scn(struct rtas_event *re)
{
    return (struct rtas_vend_errlog_scn *)get_re_scn(re, RTAS_VEND_ERRLOG_SCN);
}

/**
 * print_re_vend_specific_scn
 * @brief print the contents of a vendor specific section
 *
 * @param res rtas_event_scn to print
 * @param verbosity verbose level
 * @return number of bytes written
 */
int 
print_re_vend_errlog_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_vend_errlog *ve;
    int len = 0;

    if (shdr->scn_id != RTAS_VEND_ERRLOG_SCN)  {
        errno = EFAULT;
        return -1;
    }

    ve = (struct rtas_vend_errlog *)shdr;

    len += print_scn_title("Vendor Error Log Section");

    len += rtas_print("%-20s%c%c%c%c\n", "Vendor ID:", ve->vendor_id[0],
                      ve->vendor_id[1], ve->vendor_id[2], ve->vendor_id[3]);
    
    if (ve->vendor_data != NULL) {
        len += rtas_print("Raw Vendor Error Log:\n");
        len += print_raw_data(ve->vendor_data, ve->vendor_data_sz);
    }

    return len;
}
