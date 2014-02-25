/**
 * @file rtas_event.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#ifndef _H_RTAS_EVENT
#define _H_RTAS_EVENT

#define PRNT_FMT        "%-20s%08x"
#define PRNT_FMT_L      PRNT_FMT"    "
#define PRNT_FMT_R      PRNT_FMT"\n"
#define PRNT_FMT_2      PRNT_FMT_L PRNT_FMT_R
#define PRNT_FMT_ADDR   "%-20s%08x%08x\n"

void rtas_copy(void *, struct rtas_event *, uint32_t);

/* parse routines */
int parse_priv_hdr_scn(struct rtas_event *);
int parse_usr_hdr_scn(struct rtas_event *);
int parse_epow_scn(struct rtas_event *);
int parse_io_scn(struct rtas_event *);
int parse_dump_scn(struct rtas_event *);
int parse_lri_scn(struct rtas_event *);
int parse_mt_scn(struct rtas_event *);
int parse_src_scn(struct rtas_event *);

int parse_cpu_scn(struct rtas_event *);
int parse_ibm_diag_scn(struct rtas_event *);
int parse_mem_scn(struct rtas_event *);
int parse_post_scn(struct rtas_event *);
int parse_sp_scn(struct rtas_event *);
int parse_vend_errlog_scn(struct rtas_event *);
int parse_generic_v6_scn(struct rtas_event *);
void parse_mtms(struct rtas_event *, struct rtas_mtms *);
int parse_hotplug_scn(struct rtas_event *);

/* print routines */
int print_re_hdr_scn(struct scn_header *, int);
int print_re_exthdr_scn(struct scn_header *, int);
int print_re_epow_scn(struct scn_header *, int);
int print_re_io_scn(struct scn_header *, int);
int print_re_cpu_scn(struct scn_header *, int);
int print_re_ibm_diag_scn(struct scn_header *, int);
int print_re_mem_scn(struct scn_header *, int);
int print_re_post_scn(struct scn_header *, int);
int print_re_ibmsp_scn(struct scn_header *, int);
int print_re_vend_errlog_scn(struct scn_header *, int);
int print_re_priv_hdr_scn(struct scn_header *, int);
int print_re_usr_hdr_scn(struct scn_header *, int);
int print_re_dump_scn(struct scn_header *, int);
int print_re_lri_scn(struct scn_header *, int);
int print_re_mt_scn(struct scn_header *, int);
int print_re_src_scn(struct scn_header *, int);
int print_re_generic_scn(struct scn_header *, int);
int print_re_hotplug_scn(struct scn_header *, int);

int print_mtms(struct rtas_mtms *);

int print_scn_title(char *, ...);
int print_v6_hdr(char *, struct rtas_v6_hdr *, int);
int print_raw_data(char *, int);
int rtas_print(char *fmt, ...);
struct scn_header * get_re_scn(struct rtas_event *, int);
void add_re_scn(struct rtas_event *, void *, int);
int re_scn_id(struct rtas_v6_hdr *);

#endif
