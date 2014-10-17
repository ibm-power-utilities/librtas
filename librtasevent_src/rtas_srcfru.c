/**
 * @file rtas_srcfru.c
 * @brief RTAS SRC/FRU sections routines
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

/* rtas_src_codes contains data used by these routines */ 
#include "rtas_src_codes.c"

/**
 * parse_fru_hdr
 * @brief Parse the ontents of a FRU header
 *
 * @param fru_hdr rtas_fru_hdr pointer
 * @param fru_hdr_raw rtas_fru_hdr_raw pointer
 */
static void parse_fru_hdr(struct rtas_fru_hdr *fru_hdr,
			  struct rtas_fru_hdr_raw *fru_hdr_raw)
{
    fru_hdr->id[0] = fru_hdr_raw->id[0];
    fru_hdr->id[1] = fru_hdr_raw->id[1];

    fru_hdr->length = fru_hdr_raw->length;
    fru_hdr->flags = fru_hdr_raw->flags;
}

/**
 * parse_fru_id_scn
 * @brief Parse a FRU Identity Substructure
 *
 * @param re rtas_event pointer
 * @returns pointer to parsed rtas_fru_id_scn, NULL on failure
 */
static struct rtas_fru_hdr *
parse_fru_id_scn(struct rtas_event *re)
{
    struct rtas_fru_id_scn *fru_id;
    struct rtas_fru_id_scn_raw *fru_id_raw;

    fru_id = malloc(sizeof(*fru_id));
    if (fru_id == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset(fru_id, 0, sizeof(*fru_id));

    fru_id_raw = (struct rtas_fru_id_scn_raw *)(re->buffer + re->offset);
    parse_fru_hdr(&fru_id->fruhdr, &fru_id_raw->fruhdr);
    re->offset += RE_FRU_HDR_SZ;

    if (fruid_has_part_no(fru_id)) {
        strcpy(fru_id->part_no, RE_EVENT_OFFSET(re));
        re->offset += 8;
    }

    if (fruid_has_proc_id(fru_id)) {
        strcpy(fru_id->procedure_id, RE_EVENT_OFFSET(re));
        re->offset += 8;
    }

    if (fruid_has_ccin(fru_id)) {
        rtas_copy(fru_id->ccin, re, 4);
        fru_id->ccin[4] = '\0';
    }
            
    if (fruid_has_serial_no(fru_id)) {
        rtas_copy(fru_id->serial_no, re, 12);
        fru_id->serial_no[12] = '\0';
    }

    return (struct rtas_fru_hdr *)fru_id;
}

/**
 * parse_fru_pe_scn
 * @brief parse a FRU Power Enclosure Identity Substructure
 *
 * @param re rtas_event pointer
 * @returns pointer to parsed rtas_fru_pe_scn, NULL on failure
 */
static struct rtas_fru_hdr *
parse_fru_pe_scn(struct rtas_event *re)
{
    struct rtas_fru_pe_scn *fru_pe;
    struct rtas_fru_pe_scn_raw *fru_pe_raw;
    uint32_t scn_sz;
    char *data;

    fru_pe = malloc(sizeof(*fru_pe));
    if (fru_pe == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset(fru_pe, 0, sizeof(*fru_pe));
    fru_pe_raw = (struct rtas_fru_pe_scn_raw *)(re->buffer + re->offset);
    parse_fru_hdr(&fru_pe->fruhdr, &fru_pe_raw->fruhdr);
    re->offset += RE_FRU_HDR_SZ;

    scn_sz = fru_pe->fruhdr.length;
    data = (char *)fru_pe + sizeof(fru_pe->fruhdr);
    
    rtas_copy(data, re, scn_sz - RE_FRU_HDR_SZ);

    return (struct rtas_fru_hdr *)fru_pe;
}
    
/**
 * parse_fru_mr_scn
 * @brief parse a FRU Manufacturing Replaceable Unit Substructure
 *
 * @param re rtas_event pointer
 * @returns pointer to parsed rtas_fru_mr_scn, NULL on failure
 */
static struct rtas_fru_hdr *
parse_fru_mr_scn(struct rtas_event *re)
{
    struct rtas_fru_mr_scn *fru_mr;
    struct rtas_fru_mr_scn_raw *fru_mr_raw;
    int i, mrus_sz, num_mrus;

    fru_mr = malloc(sizeof(*fru_mr));
    if (fru_mr == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset(fru_mr, 0, sizeof(*fru_mr));
    fru_mr_raw = (struct rtas_fru_mr_scn_raw *)(re->buffer + re->offset);
    parse_fru_hdr(&fru_mr->fruhdr, &fru_mr_raw->fruhdr);
    re->offset += RE_FRU_HDR_SZ;

    mrus_sz = fru_mr->fruhdr.length - RE_FRU_HDR_SZ - sizeof(uint32_t);
    num_mrus = mrus_sz / sizeof(struct fru_mru_raw);

    for (i = 0; i < num_mrus; i++) {
	fru_mr->mrus[i].priority = fru_mr_raw->mrus[i].priority;
	fru_mr->mrus[i].id = be32toh(fru_mr_raw->mrus[i].id);
    }

    re->offset += mrus_sz + sizeof(uint32_t) /* reserved piece */; 
    return (struct rtas_fru_hdr *)fru_mr;
}

void parse_fru_scn(struct rtas_event *re, struct rtas_fru_scn *fru,
		   struct rtas_fru_scn_raw *rawfru)
{
    fru->length = rawfru->length;

    fru->type = (rawfru->data1 & 0xf0) >> 4;
    fru->fru_id_included = (rawfru->data1 & 0x08) >> 3;
    fru->fru_subscn_included = rawfru->data1 & 0x07;

    fru->priority = rawfru->priority;
    fru->loc_code_length = rawfru->loc_code_length;
    re->offset += RE_FRU_SCN_SZ;

    memcpy(fru->loc_code, rawfru->loc_code, fru->loc_code_length);
    re->offset += fru->loc_code_length;
}

/**
 * parse_v6_src_scn
 * @brief parse a version 6 rtas SRC section
 *
 * @param re rtas_event pointer
 * @param src_start pointer to beginning of SRC section
 * @return 0 on success, !0 on failure
 */
int
parse_src_scn(struct rtas_event *re)
{
    struct rtas_src_scn *src;
    struct rtas_src_scn_raw *src_raw;
    struct rtas_fru_scn *fru, *last_fru;
    int total_len, srcsub_len;
    src = malloc(sizeof(*src));
    if (src == NULL) {
        errno = ENOMEM;
        return 1;
    }

    src_raw = malloc(sizeof(*src_raw));
    if (src_raw == NULL) {
        errno = ENOMEM;
        return 1;
    }
           
    memset(src, 0, sizeof(*src));
    memset(src_raw, 0, sizeof(*src_raw));
    src->shdr.raw_offset = re->offset;

    rtas_copy(src_raw, re, RE_SRC_SCN_SZ);
    parse_v6_hdr(&src->v6hdr, &src_raw->v6hdr);

    src->version = src_raw->version;
    memcpy(&src->src_platform_data, &src_raw->src_platform_data,
	   sizeof(src->src_platform_data));

    src->ext_refcode2 = be32toh(src_raw->ext_refcode2);
    src->ext_refcode3 = be32toh(src_raw->ext_refcode3);
    src->ext_refcode4 = be32toh(src_raw->ext_refcode4);
    src->ext_refcode5 = be32toh(src_raw->ext_refcode5);
    
    src->ext_refcode6 = be32toh(src_raw->ext_refcode6);
    src->ext_refcode7 = be32toh(src_raw->ext_refcode7);
    src->ext_refcode8 = be32toh(src_raw->ext_refcode8);
    src->ext_refcode9 = be32toh(src_raw->ext_refcode9);

    memcpy(&src->primary_refcode, &src_raw->primary_refcode,
	   sizeof(src->primary_refcode));

    add_re_scn(re, src, re_scn_id(&src_raw->v6hdr));

    if (!src_subscns_included(src))
        return 0;

    rtas_copy( (char *) src_raw + RE_SRC_SCN_SZ + 4, re, RE_SRC_SUBSCN_SZ);

    src->subscn_id = src_raw->subscn_id;
    src->subscn_platform_data = src_raw->subscn_platform_data;
    src->subscn_length = be16toh(src_raw->subscn_length);

    srcsub_len = src->subscn_length * 4; /*get number of bytes */
    total_len = RE_SRC_SUBSCN_SZ;

    last_fru = NULL;

    do {
	uint32_t fru_len, fru_end;
	struct rtas_fru_hdr *last_fruhdr = NULL;
	struct rtas_fru_scn_raw *rawfru;
        fru = malloc(sizeof(*fru));
        if (fru == NULL) {
            cleanup_rtas_event(re);
            errno = ENOMEM;
            return 1;
        }

        memset(fru, 0, sizeof(*fru));

	rawfru = (struct rtas_fru_scn_raw *)(re->buffer + re->offset);
	parse_fru_scn(re, fru, rawfru);

	fru_len = RE_FRU_SCN_SZ + fru->loc_code_length;
        fru_end = re->offset + fru->length - fru_len;

	while (re->offset < fru_end) {
	    struct rtas_fru_hdr *cur_fruhdr = NULL;
	    char *id = re->buffer + re->offset;

            if (strncmp(id, "ID", 2) == 0)
                cur_fruhdr = parse_fru_id_scn(re);
            else if (strncmp(id, "PE", 2) == 0)
                cur_fruhdr = parse_fru_pe_scn(re);
            else if (strncmp(id, "MR", 2) == 0)
                cur_fruhdr = parse_fru_mr_scn(re);
            else {
                re->offset++;
                continue;
            }

            if (cur_fruhdr == NULL) {
                cleanup_rtas_event(re);
                return -1;
            }

            if (last_fruhdr == NULL)
                fru->subscns = cur_fruhdr;
            else
                last_fruhdr->next = cur_fruhdr;

            last_fruhdr = cur_fruhdr;
        }

        if (last_fru == NULL) 
            src->fru_scns = fru;
        else 
            last_fru->next = fru;

        last_fru = fru;
        
        total_len += fru->length;
    } while (total_len < srcsub_len);

    return 0;
}

/**
 * rtas_get_src_scn
 * @brief retrieve the RTAS src section for a RTAS event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for a SRC section
 */ 
struct rtas_src_scn *
rtas_get_src_scn(struct rtas_event *re)
{
    return (struct rtas_src_scn *)get_re_scn(re, RTAS_PSRC_SCN);
}

/**
 * print_fru_hdr
 * @brief print the contents of a FRU header
 *
 * @param fruhdr pointer to fru_hdr to print
 * @param verbosity verbose level
 * @returns the number of bytes printed
 */
static int
print_fru_hdr(struct rtas_fru_hdr *fruhdr, int verbosity)
{
    int len = 0;

    len += rtas_print("%-20s%c%c          "PRNT_FMT_R, "ID:", fruhdr->id[0],
                      fruhdr->id[1], "Flags:", fruhdr->flags);

    if (verbosity >= 2) 
        len += rtas_print(PRNT_FMT_R, "Length:", fruhdr->length);

    return len;
}

/**
 * print_fru_priority
 * @brief decode the FRU prority level and print a description of it.
 *
 * @param priority
 * @returns the number of bytes printed
 */
static int
print_fru_priority(char priority)
{
    int len = 0;

    len = rtas_print("%-20s%c - ", "Priority:", priority);
    switch (priority) {
        case RTAS_FRU_PRIORITY_HIGH:
            len += rtas_print("High priority and mandatory call-out.\n");
            break;
            
        case RTAS_FRU_PRIORITY_MEDIUM:
            len += rtas_print("Medium priority.\n");
            break;
            
        case RTAS_FRU_PRIORITY_MEDIUM_A:
            len += rtas_print("Medium priority group A (1st group).\n");
            break;
            
        case RTAS_FRU_PRIORITY_MEDIUM_B:
            len += rtas_print("Medium priority group B (2nd group).\n");
            break;
            
        case RTAS_FRU_PRIORITY_MEDIUM_C:
            len += rtas_print("Medium priority group C (3rd group).\n");
            break;
            
        case RTAS_FRU_PRIORITY_LOW:
            len += rtas_print("Low Priority.\n");
            break;
    }

    return len;
}

/**
 * print_fru_id_scn
 * @bried print the contents of a FRU Identity substructure
 *
 * @param fruhdr pointer to the fru_hdr of the FRU ID section to print
 * @param verbosity verbose level
 * @returns the number of bytes printed
 */
static int
print_fru_id_scn(struct rtas_fru_hdr *fruhdr, int verbosity)
{
    struct rtas_fru_id_scn  *fru_id = (struct rtas_fru_id_scn *)fruhdr;
    int len;
    uint32_t component;

    len = print_scn_title("FRU ID Section");
    len += print_fru_hdr(fruhdr, verbosity);

    component = fru_id->fruhdr.flags & RTAS_FRUID_COMP_MASK;

    if (component) {
        len += rtas_print(PRNT_FMT" ", "Failing Component:", component);
            
        switch (component) {
            case RTAS_FRUID_COMP_HARDWARE:
                len += rtas_print("(\"normal\" hardware FRU)\n");
                break;
                
            case RTAS_FRUID_COMP_CODE:
                len += rtas_print("(Code FRU)\n");
                break;
                
            case RTAS_FRUID_COMP_CONFIG_ERROR:
                len += rtas_print("(Configuration error)\n");
                break;
                
            case RTAS_FRUID_COMP_MAINT_REQUIRED:
                len += rtas_print("(Mainteneace procedure required)\n");
                break;
                
            case RTAS_FRUID_COMP_EXTERNAL: 
                len += rtas_print("(External FRU)\n");
                break;
                
            case RTAS_FRUID_COMP_EXTERNAL_CODE:
                len += rtas_print("(External Code FRU)\n");
                break;
                
            case RTAS_FRUID_COMP_TOOL:
                len += rtas_print("(Tool FRU)\n");
                break;
                
            case RTAS_FRUID_COMP_SYMBOLIC:
                len += rtas_print("(Symbolic FRU)\n");
                break;
                
            default:
                len += rtas_print("\n");
                break;
        }
    }

    if (fruid_has_part_no(fru_id))
        len += rtas_print("%-20s%s\n", "FRU Stocking Part:", fru_id->part_no);

    if (fruid_has_proc_id(fru_id))
        len += rtas_print("%-20s%s\n", "Procedure ID:", fru_id->procedure_id);

    if (fruid_has_ccin(fru_id))
        len += rtas_print("%-20s%s\n", "CCIN:", fru_id->ccin);

    if (fruid_has_serial_no(fru_id))
        len += rtas_print("%-20s%s\n", "Serial Number:", fru_id->serial_no);

    len += rtas_print("\n");
    return len;
}

/**
 * print_fru_pe_scn
 * @bried print the contents of a FRU Power Enclosure substructure
 *
 * @param fruhdr pointer to the fru_hdr of the FRU PE section to print
 * @param verbosity verbose level
 * @returns the number of bytes printed
 */
static int
print_fru_pe_scn(struct rtas_fru_hdr *fruhdr, int verbosity)
{
    struct rtas_fru_pe_scn  *fru_pe = (struct rtas_fru_pe_scn *)fruhdr;
    int len;

    len = print_scn_title("FRU PE Section");
    len += print_fru_hdr(fruhdr, verbosity);
    len += print_mtms(&fru_pe->pce_mtms);

    if (fru_pe->pce_name[0] != '\0')
        len += rtas_print("%-20s%s\n\n", "PCE Name:", fru_pe->pce_name);
    else
        len += rtas_print("\n\n");

    return len;
}

/**
 * print_fru_mr_scn
 * @bried print the contents of a FRU Manufacturing Replaceable substructure
 *
 * @param fruhdr pointer to the fru_hdr of the FRU MR section to print
 * @param verbosity verbose level
 * @returns the number of bytes printed
 */
static int
print_fru_mr_scn(struct rtas_fru_hdr *fruhdr, int verbosity)
{
    struct rtas_fru_mr_scn  *fru_mr = (struct rtas_fru_mr_scn *)fruhdr;
    int i, len;

    len = print_scn_title("FRU MR Section");
    len += print_fru_hdr(fruhdr, verbosity);

    len += rtas_print("\nManufacturing Replaceable Unit Fields (%d):\n",
		      frumr_num_callouts(fru_mr));
    for (i = 0; i < frumr_num_callouts(fru_mr); i++) {
        struct fru_mru *mru = &fru_mr->mrus[i];

        len += rtas_print("%-20s%c           %-20s%08x\n", "MRU Priority:", 
			  mru->priority, "MRU ID:", mru->id);
    }

    len += rtas_print("\n");
    return len;
}
        
/**
 * print_re_fru_scn
 * @brief print the contents of an FRU section
 *
 * @param res rtas_event_scn pointer for a fru section
 * @param verbosity verbose level of output
 * @param count current fru section number that we are printing
 * @return number of bytes written
 */
int
print_re_fru_scn(struct rtas_fru_scn *fru, int verbosity, int count)
{
    struct rtas_fru_hdr *fruhdr;
    int len = 0;

    len += print_scn_title("FRU Section (%d)", count);

    if (verbosity >= 2) {
        len += rtas_print(PRNT_FMT_2, "Length:", fru->length, 
	                  "Call-Out Type:", fru->type);
        
        len += rtas_print("%-20s%-8s    %-20s%-8s\n", "Fru ID Included:",
                          (fru->fru_id_included) ? "Yes" : "No", 
                          "Fru Subscns:", 
                          (fru->fru_subscn_included) ? "Yes" : "No");
    }

    len += print_fru_priority(fru->priority);

    if (fru->loc_code_length) {
        if (verbosity >= 2)
            len += rtas_print(PRNT_FMT_R, "Loc Code Length:", 
                              fru->loc_code_length);
        
        len += rtas_print("%-20s%s\n", "Location Code:", fru->loc_code);
    }

    len += rtas_print("\n");

    for (fruhdr = fru->subscns; fruhdr != NULL; fruhdr = fruhdr->next) {
        if (strncmp(fruhdr->id, "ID", 2) == 0) 
            len += print_fru_id_scn(fruhdr, verbosity);
        else if (strncmp(fruhdr->id, "PE", 2) == 0)
            len += print_fru_pe_scn(fruhdr, verbosity);
        else if (strncmp(fruhdr->id, "MR", 2) == 0)
            len += print_fru_mr_scn(fruhdr, verbosity);
    }

    return len;
}


/**
 * print_src_refcode
 * @brief print a detailed description of the SRC reference code
 *
 * @param src rtas_v6_src_scn pointer
 * @return number of bytes written
 */
int
print_src_refcode(struct rtas_src_scn *src)
{
    int i, len = 0;

    len += rtas_print("%s \"", "Primary Reference Code:");
    for (i = 0; i < 32; i++) {
        if (src->primary_refcode[i] == '\0') 
            break;
        len += rtas_print("%c", src->primary_refcode[i]);
    }
    len += rtas_print("\"\n");

    i = 0;
    while (src_codes[i].desc != NULL) {
        if (strcmp(src->primary_refcode, src_codes[i].id) == 0) {
            len += rtas_print("%s\n", src_codes[i].desc);
            return len;
        }
        i++;
    }
    
    return len;
}

/**
 * print_re_src_scn
 * @brief print the contents of a SRC section
 *
 * @param res rtas_event_scn pointer for SRC section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_src_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_src_scn *src;
    struct rtas_fru_scn *fru;
    int len = 0;
    int count = 1;

    if ((shdr->scn_id != RTAS_PSRC_SCN) && (shdr->scn_id != RTAS_SSRC_SCN)) {
        errno = EFAULT;
        return 0;
    }

    src = (struct rtas_src_scn *)shdr;

    if (strncmp(src->v6hdr.id, RTAS_PSRC_SCN_ID, 2) == 0)
        len += print_v6_hdr("Primary SRC Section",
			    (struct rtas_v6_hdr *)&src->v6hdr, verbosity);
    else
        len += print_v6_hdr("Secondary SRC Section",
			    (struct rtas_v6_hdr *)&src->v6hdr, verbosity);

    if (verbosity >= 2) {
        len += rtas_print(PRNT_FMT_2"\n", "SRC Version:", src->version,
                          "Subsections:", src_subscns_included(src));
    }

    len += rtas_print("Platform Data:\n");
    len += print_raw_data((char*)src->src_platform_data, 
                          sizeof(src->src_platform_data));
    len += rtas_print("\n");

    len += rtas_print("Extended Reference Codes:\n");
    len += rtas_print("2: %08x  3: %08x  4: %08x  5: %08x\n", 
                      src->ext_refcode2, src->ext_refcode3, 
                      src->ext_refcode4, src->ext_refcode5);
    len += rtas_print("6: %08x  7: %08x  8: %08x  9: %08x\n\n", 
                      src->ext_refcode6, src->ext_refcode7, 
                      src->ext_refcode8, src->ext_refcode9);

    len += print_src_refcode(src);

    if (src_subscns_included(src)) {
        if (verbosity >= 2) {
            len += rtas_print(PRNT_FMT_2, "Sub-Section ID:", src->subscn_id,
                              "Platform Data:", src->subscn_platform_data);
            len += rtas_print(PRNT_FMT_R, "Length:", src->subscn_length);
        }
    }

    len += rtas_print("\n");

    for (fru = src->fru_scns; fru != NULL; fru = fru->next) {
        len += print_re_fru_scn(fru, verbosity, count);
        count++;
    }

    return len;
}
