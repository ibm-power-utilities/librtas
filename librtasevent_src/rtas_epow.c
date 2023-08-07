/**
 * @file rtas_epow.c
 * @brief RTAS event EPOW section routines
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
 * parse_epow_scn
 *
 */
int
parse_epow_scn(struct rtas_event *re)
{
    struct rtas_epow_scn *epow;
    struct rtas_v6_epow_scn_raw *rawhdr;

    epow = malloc(sizeof(*epow));
    if (epow == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(epow, 0, sizeof(*epow));

    epow->shdr.raw_offset = re->offset;
    
    if (re->version < 6) {
        rtas_copy(RE_SHDR_OFFSET(epow), re, RE_V4_SCN_SZ);
    } else {
	rawhdr = (struct rtas_v6_epow_scn_raw *)(re->buffer + re->offset);

	parse_v6_hdr(&epow->v6hdr, &rawhdr->v6hdr);
	epow->sensor_value = (rawhdr->data1 & 0xF0) >> 4;
	epow->action_code = rawhdr->data1 & 0x0F;
	epow->event_modifier = rawhdr->event_modifier;

	memcpy(epow->reason_code, rawhdr->reason_code, 8);
	re->offset += RE_EPOW_V6_SCN_SZ;
    }

    add_re_scn(re, epow, RTAS_EPOW_SCN);

    return 0;
}

/**
 * rtas_get_epow_scn
 * @brief Retrieve the Environmental and Power Warning (EPOW) section
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for the epow section
 */
struct rtas_epow_scn *
rtas_get_epow_scn(struct rtas_event *re)
{
    return (struct rtas_epow_scn *)get_re_scn(re, RTAS_EPOW_SCN);
}


/**
 * print_v4_epow
 * @brief print the contents of a pre-version 6 EPOW section
 *
 * @param res rtas_event_scn pointer for epow section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
static int
print_v4_epow(struct scn_header *shdr, int verbosity)
{
    struct rtas_epow_scn *epow = (struct rtas_epow_scn *)shdr;
    int version = shdr->re->version;
    int len = 0;

    len += print_scn_title("EPOW Warning");
    len += rtas_print(PRNT_FMT_R, "EPOW Sensor Value:", (uint32_t)epow->sensor_value); 

    if (version >= 3) {
        if (epow->sensor) {
	    len += rtas_print("EPOW detected by a sensor\n");
            len += rtas_print(PRNT_FMT_2, 
                              "Sensor Token:", epow->sensor_token, 
                              "Sensor Index:", epow->sensor_index);
            len += rtas_print(PRNT_FMT_2,
                              "Sensor Value:", (uint32_t)epow->sensor_value,
                              "Sensor Status:", epow->sensor_status);
        }

        if (epow->power_fault) 
	    len += rtas_print("EPOW caused by a power fault.\n");
        if (epow->fan) 
	    len += rtas_print("EPOW caused by fan failure.\n");
        if (epow->temp) 
	    len += rtas_print("EPOW caused by over-temperature condition.\n");
        if (epow->redundancy) 
	    len += rtas_print("EPOW warning due to loss of redundancy.\n");
        if (epow->CUoD) 
	    len += rtas_print("EPOW warning due to CUoD Entitlement "
                              "Exceeded.\n");
        if (epow->general) 
	    len += rtas_print("EPOW general power fault.\n");
        if (epow->power_loss) 
	    len += rtas_print("EPOW power fault due to loss of power "
                              "source.\n");
        if (epow->power_supply) 
	    len += rtas_print("EPOW power fault due to internal power "
                              "supply failure.\n");
        if (epow->power_switch) 
	    len += rtas_print("EPOW power fault due to activation of "
                              "power switch.\n");
    }

    if ((version == 4) && (epow->battery))
	len += rtas_print("EPOW power fault due to internal battery "
                          "failure.\n");

    len += rtas_print("\n");

    return len;
}

/**
 * print_v6_epow
 * @brief print the contents of a RTAS version 6 EPOW section
 *
 * @param res rtas_event_scn pointer for epow section
 * @param verbosity verbose level of output
 * @return number of bytes written 
 */
static int
print_v6_epow(struct scn_header *shdr, int verbosity)
{
    struct rtas_epow_scn *epow = (struct rtas_epow_scn *)shdr; 
    int len = 0;

    len += print_v6_hdr("EPOW Warning", &epow->v6hdr, verbosity);

    len += rtas_print(PRNT_FMT_2, "Sensor Value:", (uint32_t)epow->sensor_value,
                      "Action Code:", (uint32_t)epow->action_code);
    len += rtas_print(PRNT_FMT_R, "EPOW Event:", epow->event_modifier);

    switch (epow->event_modifier) {
        case RTAS_EPOW_MOD_NA:
	    break;
            
        case RTAS_EPOW_MOD_NORMAL_SHUTDOWN:
	    len += rtas_print(" - Normal System Shutdown with no "
                              "additional delay.\n");
	    break;
            
        case RTAS_EPOW_MOD_UTILITY_POWER_LOSS:
	    len += rtas_print(" - Loss of utility power, system is "
                              "running on UPS/battery.\n");
	    break;
            
        case RTAS_EPOW_MOD_CRIT_FUNC_LOSS:
	    len += rtas_print(" - Loss of system critical functions, "
                              "system should be shutdown.\n");
	    break;

	case RTAS_EPOW_MOD_AMBIENT_TEMP:
	    len += rtas_print(" - Ambient temperature too high, "
                              "system should be shutdown.\n");
	    break;

	default:
	    len += rtas_print(" - Unknown action code.\n");
    }

    len += rtas_print("Platform specific reason code:");
    len += print_raw_data(epow->reason_code, 8);
    len += rtas_print("\n");

    return len;
}

/**
 * print_re_epow_scn
 * @brief print the contents of a RTAS EPOW section
 *
 * @param res rtas_event_scn pointer to epow section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_epow_scn(struct scn_header *shdr, int verbosity)
{
    if (shdr->scn_id != RTAS_EPOW_SCN) {
        errno = EFAULT;
        return 0;
    }

    if (shdr->re->version == 6)
        return print_v6_epow(shdr, verbosity);
    else
        return print_v4_epow(shdr, verbosity);
}

