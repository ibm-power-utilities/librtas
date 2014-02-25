/**
 * @file get_rtas_event.c
 * @brief librtasevent routines to parse RTAS events
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * rtas_copy
 * @brief front end for common memcpy calls
 *
 * NOTE: This will chang the value of the rtas_event offset value
 * 
 * @param to place to copy data to
 * @param re rtas_event pointer
 * @param size amount to copy
 */
void
rtas_copy(void *to, struct rtas_event *re, uint32_t size)
{
	memcpy(to, (re->buffer + re->offset), size);
	re->offset += size;
}

/**
 * cleanup_rtas_event
 * @brief free the structures related to a parsed rtas event
 *
 * @param re rtas_event pointer
 * @return 0 on success, !0 on failure
 */
int
cleanup_rtas_event(struct rtas_event *re)
{
    struct scn_header *scn;

    if (re == NULL)
        return 0;

    scn = re->event_scns;

    while (scn != NULL) {
        struct scn_header *tmpscn = scn->next;

        switch (scn->scn_id) {
            case RTAS_VEND_ERRLOG_SCN:
            {
                struct rtas_vend_errlog *ve = (struct rtas_vend_errlog *)scn;
            
                if (ve->vendor_data != NULL)
                    free(ve->vendor_data);
            
                break;
            }

            case RTAS_PSRC_SCN:
            case RTAS_SSRC_SCN:
            {
                struct rtas_fru_hdr *fruhdr;
                struct rtas_fru_scn *fru;
                struct rtas_src_scn *src = (struct rtas_src_scn *)scn;

                fru = src->fru_scns;
                while (fru != NULL) {
                    struct rtas_fru_scn *tmpfru = fru->next;
                    
                    fruhdr = fru->subscns;
                    while (fruhdr != NULL) {
                        struct rtas_fru_hdr *tmphdr = fruhdr->next;
                        free(fruhdr);
                        fruhdr = tmphdr;
                    }

                    free(fru);
                    fru = tmpfru;
                }

                break;
            }

            case RTAS_GENERIC_SCN:
            {
                struct rtas_v6_generic *gen = (struct rtas_v6_generic *)scn;

                if (gen->data != NULL)
                    free(gen->data);
                    
                break;
            }
        }

        free(scn);
        scn = tmpscn;
    }

    free(re);

    return 0;
}

/**
 * add_re_scn
 * @brief Add a rtas event section to the section list
 *
 * @param re rtas_event pointer to add this section to
 * @param scn pointer to start of rtas event section
 * @param scn_id id of the section to be added
 * return pointer to newly created rtas_event_scn
 */
void
add_re_scn(struct rtas_event *re, void *scn, int scn_id)
{
    struct scn_header *shdr = (struct scn_header *)scn;

    shdr->next = NULL;
    shdr->re = re;
    shdr->scn_id = scn_id;

    if (re->event_scns == NULL) {
        re->event_scns = shdr;
    } 
    else {
        struct scn_header *tmp_hdr = re->event_scns;
        while (tmp_hdr->next != NULL)
            tmp_hdr = tmp_hdr->next;

        tmp_hdr->next = shdr;
    }
}

/**
 * re_scn_id
 * @brief Convert the two character section id into an internal identifier
 *
 * @param shdr rtas_v6_hdr pointer
 * @return section identifier on success, -1 on failure
 */
int
re_scn_id(struct rtas_v6_hdr *v6hdr) 
{
    if (strncmp(v6hdr->id, RTAS_DUMP_SCN_ID, 2) == 0)
        return RTAS_DUMP_SCN;
    if (strncmp(v6hdr->id, RTAS_EPOW_SCN_ID, 2) == 0)
        return RTAS_EPOW_SCN;
    if (strncmp(v6hdr->id, RTAS_IO_SCN_ID, 2) == 0)
        return RTAS_IO_SCN;
    if (strncmp(v6hdr->id, RTAS_LRI_SCN_ID, 2) == 0)
        return RTAS_LRI_SCN;
    if (strncmp(v6hdr->id, RTAS_MTMS_SCN_ID, 2) == 0)
        return RTAS_MT_SCN;
    if (strncmp(v6hdr->id, RTAS_PSRC_SCN_ID, 2) == 0)
        return RTAS_PSRC_SCN;
    if (strncmp(v6hdr->id, RTAS_SSRC_SCN_ID, 2) == 0)
        return RTAS_SSRC_SCN;
    if (strncmp(v6hdr->id, RTAS_HP_SCN_ID, 2) == 0)
	return RTAS_HP_SCN;

    return -1;
}

/**
 * _get_re_scn
 * @brief the real work for retreiving rtas event sections
 *
 * @param res rtas_event_scn pointer at which to start search
 * @param scn_id section id to search for
 * @return rtas_event_scn on success, NULL on failure
 */
static struct scn_header *
_get_re_scn(struct scn_header *scn_hdr, int scn_id)
{
    for ( ; scn_hdr != NULL; scn_hdr = scn_hdr->next) {
        if (scn_hdr->scn_id == scn_id)
            return scn_hdr;
    }

    return NULL;
}

/**
 * get_re_scn
 * @brief find the specified section on the list of sections
 *
 * NOTE: this function has been split just so we can have common place
 * to check for NULL 're' pointers (no sense checking everywhere we
 * call this). 
 *
 * @param re rtas_event pointer
 * @param scn_id id of the section to find
 * @return rtas_event_scn on success, NULL on failure
 */
struct scn_header *
get_re_scn(struct rtas_event *re, int scn_id)
{
    if (re == NULL) {
        errno = EFAULT;
        return NULL;
    }

    return _get_re_scn(re->event_scns, scn_id);
}


/**
 * parse_v6_rtas_event
 * @brief parse a version 6 RTAS event
 *
 * @param re rtas_event pointer
 * @return rtas_event pointer on success, NULL on failure
 */
struct rtas_event *
parse_v6_rtas_event(struct rtas_event *re)
{
    struct rtas_v6_hdr *v6hdr;
    char *ibm;
    
    ibm = re->buffer + re->offset;

    /* Sanity Check for "IBM" string */
    if (strncmp(ibm, "IBM", 3) != 0) {
        cleanup_rtas_event(re);
        errno = EFAULT;
        return NULL;
    }

    re->offset += 4; /* IBM + NULL */

    if (parse_priv_hdr_scn(re) != 0) {
        cleanup_rtas_event(re);
        return NULL;
    }

    if (parse_usr_hdr_scn(re) != 0) {
        cleanup_rtas_event(re);
        return NULL;
    }

    while (re->offset < re->event_length) {
        int scn_id, rc;
        v6hdr = (struct rtas_v6_hdr *)(re->buffer + re->offset);

        scn_id = re_scn_id(v6hdr);

        switch (scn_id) {
            case RTAS_EPOW_SCN:
                rc = parse_epow_scn(re);
                break;

            case RTAS_IO_SCN:
                rc = parse_io_scn(re);
                break;

            case RTAS_DUMP_SCN:
                rc = parse_dump_scn(re);
                break;

            case RTAS_LRI_SCN:
                rc = parse_lri_scn(re);
                break;
                
            case RTAS_MT_SCN:
                rc = parse_mt_scn(re);
                break;

            case RTAS_PSRC_SCN:
            case RTAS_SSRC_SCN:
                rc = parse_src_scn(re);
                break;

	    case RTAS_HP_SCN:
		rc = parse_hotplug_scn(re);
		break;
        
            default:
                rc = parse_generic_v6_scn(re);
                break;
        }

        if (rc) {
            cleanup_rtas_event(re);
            re = NULL;
            break;
        }
    }

    return re;
}

/**
 * parse_rtas_event
 * @brief parse an rtas event creating a populated rtas_event structure
 *
 * @param buf buffer containing the binary RTAS event
 * @param buflen length of the buffer 'buf'
 * @return pointer to rtas_event
 */
struct rtas_event *
parse_rtas_event(char *buf, int buflen)
{
    struct rtas_event *re;
    struct rtas_event_hdr *re_hdr;
    struct rtas_event_exthdr *rex_hdr;
    int rc;

    re = malloc(sizeof(*re));
    if (re == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset(re, 0, sizeof(*re));
    re->buffer = buf;
    re->event_no = -1;

    re_hdr = malloc(sizeof(*re_hdr));
    if (re_hdr == NULL) {
	    cleanup_rtas_event(re);
	    errno = ENOMEM;
	    return NULL;
    }

    rtas_copy(RE_SHDR_OFFSET(re_hdr), re, RE_EVENT_HDR_SZ);
    add_re_scn(re, re_hdr, RTAS_EVENT_HDR);

    /* Validate the length of the buffer passed in. */
    re->event_length = re_hdr->ext_log_length + RE_EVENT_HDR_SZ;
    if (re->event_length > buflen) {
        cleanup_rtas_event(re);
        return NULL;
    }

    re->version = re_hdr->version;

    if (re_hdr->extended == 0)
        return re;
    
    rex_hdr = malloc(sizeof(*rex_hdr));
    if (rex_hdr == NULL) {
        cleanup_rtas_event(re);
        errno = ENOMEM;
        return NULL;
    }

    rtas_copy(RE_SHDR_OFFSET(rex_hdr), re, RE_EXT_HDR_SZ);
    add_re_scn(re, rex_hdr, RTAS_EVENT_EXT_HDR);

    if (re_hdr->version == 6) 
        return parse_v6_rtas_event(re);


    switch (rex_hdr->format_type) {
        case RTAS_EXTHDR_FMT_CPU:
            rc = parse_cpu_scn(re);
	    break;

        case RTAS_EXTHDR_FMT_EPOW:
            rc = parse_epow_scn(re);
            break;

        case RTAS_EXTHDR_FMT_IBM_DIAG:
            rc = parse_ibm_diag_scn(re);
            break;

        case RTAS_EXTHDR_FMT_IO:
            rc = parse_io_scn(re);
            break;

        case RTAS_EXTHDR_FMT_MEMORY:
            rc = parse_mem_scn(re);
            break;
	
        case RTAS_EXTHDR_FMT_POST:
            rc = parse_post_scn(re);
            break;

        case RTAS_EXTHDR_FMT_IBM_SP:
            rc = parse_sp_scn(re);
	    break;

        case RTAS_EXTHDR_FMT_VEND_SPECIFIC_1:
        case RTAS_EXTHDR_FMT_VEND_SPECIFIC_2:
            rc = parse_vend_errlog_scn(re);
            break;
            
        default:
            errno = EFAULT;
            rc = -1;
            break;
    }

    if ((rc == 0) && (re->offset < re->event_length))  
        rc = parse_vend_errlog_scn(re);

    if (rc) {
        cleanup_rtas_event(re);
        re = NULL;
    }

    return re;
}
