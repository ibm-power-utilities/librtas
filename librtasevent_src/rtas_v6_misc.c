/**
 * @file rtas_v6_misc.c
 * @brief Routines to print out various RTAS version 6 event sections
 *
 * Copyright (C) 2004 IBM Corporation
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
 * @author Nathan Fotenot <nfont@austin.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "librtasevent.h"
#include "rtas_event.h"

/**
 * months
 * @brief array of month strings
 *
 * This array is indexed wih a hex value, thats what the extra blanks
 * are for so please leave them.
 */
static char *months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                         "Aug", "Sep", "", "", "", "", "", "", "Oct", "Nov", 
                         "Dec"};

void parse_rtas_date(struct rtas_date *rtas_date,
		     struct rtas_date_raw *rawdate)
{
    rtas_date->year = be16toh(rawdate->year);
    rtas_date->month = rawdate->month;
    rtas_date->day = rawdate->day;
}

void parse_rtas_time(struct rtas_time *rtas_time, struct rtas_time_raw *rawtime)
{
    rtas_time->hour = rawtime->hour;
    rtas_time->minutes = rawtime->minutes;
    rtas_time->seconds = rawtime->seconds;
    rtas_time->hundredths = rawtime->hundredths;
}

void parse_v6_hdr(struct rtas_v6_hdr *v6hdr, struct rtas_v6_hdr_raw *rawv6)
{
    v6hdr->id[0] = rawv6->id[0];
    v6hdr->id[1] = rawv6->id[1];

    v6hdr->length = be16toh(rawv6->length);
    v6hdr->version = rawv6->version;
    v6hdr->subtype = rawv6->subtype;
    v6hdr->creator_comp_id = be16toh(rawv6->creator_comp_id);
}

/**
 * print_v6_scn_hdr
 * @brief print the generic version 6 section header
 *
 * @param name section name
 * @param shdr rtas_v6_scn_hdr pointer
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_v6_hdr(char *name, struct rtas_v6_hdr *v6hdr, int verbosity)
{
    int len;

    len = print_scn_title(name);

    if (verbosity > 1) {
        len += rtas_print("%-20s      %c%c    "PRNT_FMT_R, 
                          "Section ID:", v6hdr->id[0], v6hdr->id[1], 
                          "Section Length:", v6hdr->length);
        len += rtas_print(PRNT_FMT_2, "Version:", v6hdr->version, 
                          "Sub_type:", v6hdr->subtype);
        len += rtas_print(PRNT_FMT_R, "Component ID:", 
                          v6hdr->creator_comp_id);
    }

    return len;
}

/**
 * parse_main_a_scn
 *
 */
int
parse_priv_hdr_scn(struct rtas_event *re)
{
    struct rtas_priv_hdr_scn *privhdr;
    struct rtas_priv_hdr_scn_raw *rawhdr;

    privhdr = malloc(sizeof(*privhdr));
    if (privhdr == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(privhdr, 0, sizeof(*privhdr));
    privhdr->shdr.raw_offset = re->offset;

    rawhdr = (struct rtas_priv_hdr_scn_raw *)(re->buffer + re->offset);
    parse_v6_hdr(&privhdr->v6hdr, &rawhdr->v6hdr);
    parse_rtas_date(&privhdr->date, &rawhdr->date);
    parse_rtas_time(&privhdr->time, &rawhdr->time);

    privhdr->creator_id = rawhdr->creator_id;
    privhdr->scn_count = rawhdr->scn_count;

    privhdr->creator_subid_hi = be32toh(rawhdr->creator_subid_hi);
    privhdr->creator_subid_lo = be32toh(rawhdr->creator_subid_lo);

    privhdr->plid = be32toh(rawhdr->plid);
    privhdr->log_entry_id = be32toh(rawhdr->log_entry_id);

    /* If the creator id is 'E', the the subsystem version is in ascii,
     * copy this info to a null terminated string.
     */
    if (privhdr->creator_id == RTAS_PH_CREAT_SERVICE_PROC) {
        memcpy(privhdr->creator_subid_name, 
               &privhdr->creator_subid_hi, 8);
        privhdr->creator_subid_name[8] = '\0';
    }

    re->offset += 48;
    add_re_scn(re, privhdr, RTAS_PRIV_HDR_SCN);
    return 0;
}

/**
 * rtas_get_priv_hdr_scn
 * @brief retrieve the Private Header section of an RTAS Event 
 *
 * @param re rtas_event pointer
 * @return pointer to rtas_event_scn on success, NULL on failure
 */
struct rtas_priv_hdr_scn *
rtas_get_priv_hdr_scn(struct rtas_event *re)
{
    return (struct rtas_priv_hdr_scn *)get_re_scn(re, RTAS_PRIV_HDR_SCN);
}

/**
 * print_re_priv_hdr_scn
 * @brief print the RTAS private header section
 *
 * @param res rtas_event_scn pointer to main a section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_priv_hdr_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_priv_hdr_scn *privhdr;
    int len = 0;

    if (shdr->scn_id != RTAS_PRIV_HDR_SCN) {
        errno = EFAULT;
        return 0;
    }

    privhdr = (struct rtas_priv_hdr_scn *)shdr;

    len += print_v6_hdr("Private Header", &privhdr->v6hdr, verbosity);
    
    len += rtas_print("%-20s%x %s %x\n", "Date:", privhdr->date.day,
                      months[privhdr->date.month], privhdr->date.year);
    len += rtas_print("%-20s%x:%x:%x:%x\n", "Time:", 
                      privhdr->time.hour, privhdr->time.minutes, 
                      privhdr->time.seconds, privhdr->time.hundredths);

    len += rtas_print("%-20s", "Creator ID:");
    switch(privhdr->creator_id) {
        case 'C':
            len += rtas_print("Hardware Management Console");
            break;
            
        case 'E':
	    len += rtas_print("Service Processor");
	    break;
            
        case 'H':
	    len += rtas_print("PHyp");
	    break;
            
        case 'W':
	    len += rtas_print("Power Control");
	    break;
            
        case 'L':
	    len += rtas_print("Partition Firmware");
	    break;

        case 'S':
            len += rtas_print("SLIC");
            break;

        default:
            len += rtas_print("Unknown");
            break;
    }
    len += rtas_print(" (%c).\n", privhdr->creator_id);
    
    if (verbosity >= 2)
        len += rtas_print(PRNT_FMT_R, "Section Count:", privhdr->scn_count); 
   
    if (privhdr->creator_id == 'E')
        len += rtas_print("Creator Subsystem Name: %s.\n", 
                          privhdr->creator_subid_name);
    else
        len += rtas_print("Creator Subsystem Version: %08x%08x.\n",
                          privhdr->creator_subid_hi, privhdr->creator_subid_lo);

    len += rtas_print(PRNT_FMT_2, "Platform Log ID:", privhdr->plid,
                      "Log Entry ID:", privhdr->log_entry_id);

    len += rtas_print("\n");
    
    return len;
}

/**
 * parse_usr_hdr_scn
 *
 */
int
parse_usr_hdr_scn(struct rtas_event *re)
{
    struct rtas_usr_hdr_scn *usrhdr;
    struct rtas_usr_hdr_scn_raw *rawhdr;

    usrhdr = malloc(sizeof(*usrhdr));
    if (usrhdr == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(usrhdr, 0, sizeof(*usrhdr));
    usrhdr->shdr.raw_offset = re->offset;

    rawhdr = (struct rtas_usr_hdr_scn_raw *)(re->buffer + re->offset);
    parse_v6_hdr(&usrhdr->v6hdr, &rawhdr->v6hdr);
    
    usrhdr->subsystem_id = rawhdr->subsystem_id;
    usrhdr->event_data = rawhdr->event_data;
    usrhdr->event_severity = rawhdr->event_severity;
    usrhdr->event_type = rawhdr->event_type;
    usrhdr->action = be16toh(rawhdr->action);

    re->offset += RE_USR_HDR_SCN_SZ;
    add_re_scn(re, usrhdr, RTAS_USR_HDR_SCN);

    return 0;
}

/**
 * rtas_rtas_usr_hdr_scn
 * @brief retrieve the User Header section for an RTAS event.
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer to User Header section, NULL on failure
 */
struct rtas_usr_hdr_scn *
rtas_get_usr_hdr_scn(struct rtas_event *re)
{
    return (struct rtas_usr_hdr_scn *)get_re_scn(re, RTAS_USR_HDR_SCN);
}

/**
 * print_usr_hdr_subsystem_id
 * @brief Print the subsystem id from the Main B section
 *
 * @param mainb rtas_v6_main_b_scn pointer
 * @return number of bytes written
 */
int
print_usr_hdr_subsystem_id(struct rtas_usr_hdr_scn *usrhdr)
{
    unsigned int id = usrhdr->subsystem_id;
    int len = 0;

    len += rtas_print(PRNT_FMT" ", "Subsystem ID:", id);
    
    if ((id >= 0x10) && (id <= 0x1F))
        len += rtas_print("(Processor, including internal cache)\n"); 
    else if ((id >= 0x20) && (id <= 0x2F))
        len += rtas_print("(Memory, including external cache)\n");
    else if ((id >= 0x30) && (id <= 0x3F))
        len += rtas_print("(I/O (hub, bridge, bus))\n");
    else if ((id >= 0x40) && (id <= 0x4F))
        len += rtas_print("(I/O adapter, device and peripheral)\n"); 
    else if ((id >= 0x50) && (id <= 0x5F))
        len += rtas_print("(CEC Hardware)\n");
    else if ((id >= 0x60) && (id <= 0x6F))
        len += rtas_print("(Power/Cooling System)\n");
    else if ((id >= 0x70) && (id <= 0x79))
        len += rtas_print("(Other Subsystems)\n");
    else if ((id >= 0x7A) && (id <= 0x7F))
        len += rtas_print("(Surveillance Error)\n");
    else if ((id >= 0x80) && (id <= 0x8F))
        len += rtas_print("(Platform Firmware)\n");
    else if ((id >= 0x90) && (id <= 0x9F))
        len += rtas_print("(Software)\n");
    else if ((id >= 0xA0) && (id <= 0xAF))
        len += rtas_print("(External Environment)\n");
    else
        len += rtas_print("\n");

    return len;
}

/**
 * print_usr_hdr_event
 * @brief print the RTAS User Header section type data
 *
 * @param mainb rtas_usr_hdr_scn pointer
 * @return number of bytes written
 */
int
print_usr_hdr_event_data(struct rtas_usr_hdr_scn *usrhdr)
{
    int len = 0;

    len += rtas_print(PRNT_FMT_R, "Event Data", usrhdr->event_data);
    len += rtas_print("\n"PRNT_FMT_R, "Event Type:", usrhdr->event_type);
    
    switch (usrhdr->event_type) {
        case 0x01:
	    len += rtas_print("Miscellaneous, informational only.\n");
            break;
	case 0x08:
	    len += rtas_print("Dump notification.\n");
            break;
	case 0x10:
	    len += rtas_print("Previously reported error has been "
                              "corrected by system.\n");
            break;
	case 0x20:
	    len += rtas_print("System resources manually "
                              "deconfigured by user.\n");
            break;
	case 0x21:
	    len += rtas_print("System resources deconfigured by system due"
                              "to prior error event.\n");
            break;
	case 0x22:
	    len += rtas_print("Resource deallocation event notification.\n");
            break;
	case 0x30:
	    len += rtas_print("Customer environmental problem has "
                              "returned to normal.\n");
            break;
	case 0x40:
	    len += rtas_print("Concurrent maintenance event.\n");
            break;
	case 0x60:
	    len += rtas_print("Capacity upgrade event.\n");
            break;
	case 0x70:
	    len += rtas_print("Resource sparing event.\n");
            break;
	case 0x80:
	    len += rtas_print("Dynamic reconfiguration event.\n");
            break;
	case 0xD0:
	    len += rtas_print("Normal system/platform shutdown "
                              "or powered off.\n");
            break;
	case 0xE0:
	    len += rtas_print("Platform powered off by user without normal "
                              "shutdown.\n");
            break;
        default:
	    len += rtas_print("Unknown event type (%d).\n", usrhdr->event_type);
            break;
    }
    
    len += rtas_print("\n"PRNT_FMT_R, "Event Severity:", 
                      usrhdr->event_severity);
    switch (usrhdr->event_severity) {
        case 0x00:
            len += rtas_print("Informational or non-error event,\n");
            break;
	case 0x10:
	    len += rtas_print("Recovered error, general.\n");
	    break;
	case 0x20:
	    len += rtas_print("Predictive error, general.\n");
	    break;
	case 0x21:
	    len += rtas_print("Predictive error, degraded performance.\n");
	    break;
	case 0x22:
	    len += rtas_print("Predictive error, fault may be corrected "
                              "after platform re-IPL.\n");
	    break;
	case 0x23:
	    len += rtas_print("Predictive Error, fault may be corrected "
                              "after IPL, degraded performance.\n");
	    break;
	case 0x24:
	    len += rtas_print("Predictive error, loss of redundancy.\n");
	    break;
	case 0x40:
	    len += rtas_print("Unrecoverable error, general.\n");
	    break;
	case 0x41:
	    len += rtas_print("Unrecoverable error, bypassed with "
                              "degraded performance.\n");
	    break;
	case 0x44:
	    len += rtas_print("Unrecoverable error, bypassed with "
                              "loss of redundancy.\n");
	    break;
	case 0x45:
	    len += rtas_print("Unrecoverable error, bypassed with loss of\n" 
                              "redundancy and performance.\n");
            break;
	case 0x48:
	    len += rtas_print("Unrecoverable error, bypassed with "
                              "loss of function.\n");
            break;
	case 0x60:
	    len += rtas_print("Error on diagnostic test, general.\n");
            break;
	case 0x61:
	    len += rtas_print("Error on diagnostic test, resource may "
                              "produce incorrect results.\n");
            break;
	default:
	    len += rtas_print("Unknown event severity (%d).\n",
                              usrhdr->event_type);
            break;
    }

    len += rtas_print("\n");
    return len;
}

/**
 * print_usr_hdr_action
 * @brief print the RTAS User Header Section action data
 *
 * @param mainb rtas_v6_us_hdr_scn pointer
 * @return number of bytes written
 */
int
print_usr_hdr_action(struct rtas_usr_hdr_scn *usrhdr)
{
    int len = 0;

    len += rtas_print(PRNT_FMT" ", "Action Flag:", usrhdr->action);

    switch (usrhdr->action) {
        case 0x8000:
	    len += rtas_print("Service Action ");
	    if (usrhdr->action & 0x4000)
	        len += rtas_print("(hidden error) ");
	    if (usrhdr->action & 0x0800)
	        len += rtas_print("call home) ");
            len += rtas_print("Required.\n");
	    break;
            
	case 0x2000:
	    len += rtas_print("Report Externally, ");
	    if (usrhdr->action & 0x1000)
	        len += rtas_print("(HMC only).\n");
	    else
	        len += rtas_print("(HMC and Hypervisor).\n");
	    break;
            
	case 0x0400:
	    len += rtas_print("Error isolation incomplete,\n"
                              "                               "
                              "further analysis required.\n");
	    break;
            
	case 0x0000:
	    break;

	default:
	    len += rtas_print("Unknown action flag (0x%08x).\n", 
                              usrhdr->action);
    }

    return len;
}

/**
 * print_re_usr_hdr_scn
 * @brief print the contents of a RTAS User Header section
 *
 * @param res rtas_event_scn ponter
 * @param verbosity verbose level of ouput
 * @return number of bytes written
 */
int
print_re_usr_hdr_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_usr_hdr_scn *usrhdr;
    int len = 0;

    if (shdr->scn_id != RTAS_USR_HDR_SCN) {
        errno = EFAULT;
        return 0;
    }

    usrhdr = (struct rtas_usr_hdr_scn *)shdr;

    len += print_v6_hdr("User Header", &usrhdr->v6hdr, verbosity);
    len += print_usr_hdr_subsystem_id(usrhdr);
    len += print_usr_hdr_event_data(usrhdr);
    len += print_usr_hdr_action(usrhdr);

    len += rtas_print("\n");
    return len;
}

/**
 * parse_mtms
 *
 */
void
parse_mtms(struct rtas_event *re, struct rtas_mtms *mtms)
{
    rtas_copy(mtms->model, re, 8);
    mtms->model[8] = '\0';

    rtas_copy(mtms->serial_no, re, 12);
    mtms->serial_no[12] = '\0';
}

/**
 * print_mtms
 *
 */
int
print_mtms(struct rtas_mtms *mtms)
{
    int len;

    len = rtas_print("%-20s%s (tttt-mmm)\n", "Model/Type:", mtms->model);
    len += rtas_print("%-20s%s\n", "Serial Number:", mtms->serial_no);

    return len;
}

/**
 * parse_mt_scn
 *
 */
int
parse_mt_scn(struct rtas_event *re)
{
    struct rtas_mt_scn *mt;

    mt = malloc(sizeof(*mt));
    if (mt == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(mt, 0, sizeof(*mt));
    mt->shdr.raw_offset = re->offset;
    rtas_copy(RE_SHDR_OFFSET(mt), re, sizeof(struct rtas_v6_hdr_raw));
    
    parse_mtms(re, &mt->mtms);
    add_re_scn(re, mt, RTAS_MT_SCN);

    return 0;
}

/**
 * rtas_get_mtms_scn
 * @brief retrieve the Failing Enclosure (MTMS) section of an RTAS Event
 *
 * @param re rtas_event pointer
 * @return pointer to rtas_event_scn on success, NULL on failure
 */
struct rtas_mt_scn *
rtas_get_mt_scn(struct rtas_event *re)
{
    return (struct rtas_mt_scn *)get_re_scn(re, RTAS_MT_SCN);
}

/**
 * print_re_mt_scn
 * @brief print the contents of a Machine Type section
 *
 * @param res rtas_event_scn pointer for mtms section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_mt_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_mt_scn *mt;
    int len = 0;

    if (shdr->scn_id != RTAS_MT_SCN) {
        errno = EFAULT;
        return 0;
    }

    mt = (struct rtas_mt_scn *)shdr;
    
    len += print_v6_hdr("Machine Type", (struct rtas_v6_hdr *)&mt->v6hdr, verbosity);
    len += print_mtms(&mt->mtms);
    len += rtas_print("\n");

    return len;
}

/**
 * parse_generic_v6_scn
 *
 */
int
parse_generic_v6_scn(struct rtas_event *re)
{
    struct rtas_v6_generic  *gen;
    struct rtas_v6_hdr_raw *rawhdr;

    gen = malloc(sizeof(*gen));
    if (gen == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(gen, 0, sizeof(*gen));
    gen->shdr.raw_offset = re->offset;

    rawhdr = (struct rtas_v6_hdr_raw *)(re->buffer + re->offset);
    parse_v6_hdr(&gen->v6hdr, rawhdr);
    re->offset += RTAS_V6_HDR_SIZE;

    if (gen->v6hdr.length > RTAS_V6_HDR_SIZE) {
        uint32_t    data_sz = gen->v6hdr.length - RTAS_V6_HDR_SIZE;
        gen->data = malloc(data_sz);
        if (gen->data == NULL) {
	    free(gen);
            errno = ENOMEM;
            return -1;
        }

        memset(gen->data, 0, data_sz);
        rtas_copy(gen->data, re, data_sz);
    }

    add_re_scn(re, gen, RTAS_GENERIC_SCN);
    return 0;
}

/**
 * print_re_generic_scn
 *
 */
int
print_re_generic_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_v6_generic *gen;
    uint32_t len = 0;

    if (shdr->scn_id != RTAS_GENERIC_SCN) {
        errno = EFAULT;
        return 0;
    }

    gen = (struct rtas_v6_generic *)shdr;
    len += print_v6_hdr("Unknown Section", &gen->v6hdr, 2);
    len += rtas_print("\n");

    if (gen->data != NULL) {
        len += rtas_print("Raw Section Data:\n");
        len += print_raw_data(gen->data, 
                              gen->v6hdr.length - sizeof(struct rtas_v6_hdr_raw));
    }

    len += rtas_print("\n");
    return len;
}
