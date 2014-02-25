/**
 * @file print_rtas_event.c
 * @brief generic routines to handle printing RTAS event sections
 *
 * Copyriht (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * rtas_print_width
 * @brief character width of the librtasevent output
 *
 * The default output of librtasevent is 80 characters wide.  This can
 * be adjusted via the rtas_set_print_width() call.
 */
static int rtas_print_width = 80;

/**
 * line_offset
 * @brief current character offset into the print line
 */
static int line_offset = 0;

/**
 * ostream
 * @brief output stream for librtasevent printing
 */
static FILE *ostream;

/**
 * re_print_fns
 * @brief arrary of function pointers for printing RTAS event sections
 *
 * NOTE: the index of these print functions corresponds to the
 * definitions for the rtas event sections from librtasevent.h.  Changes
 * need to be made in both places to avoid breaking librtasevent.
 */
static int (*re_print_fns[])() = {
    NULL,
    print_re_hdr_scn,
    print_re_exthdr_scn,
    print_re_epow_scn,
    print_re_io_scn,
    print_re_cpu_scn,
    print_re_ibm_diag_scn,
    print_re_mem_scn,
    print_re_post_scn,
    print_re_ibmsp_scn,
    print_re_vend_errlog_scn,
    print_re_priv_hdr_scn,
    print_re_usr_hdr_scn,
    print_re_dump_scn,
    print_re_lri_scn,
    print_re_mt_scn,
    print_re_src_scn,
    print_re_src_scn,
    print_re_generic_scn,
    print_re_hotplug_scn
};

/**
 * rtas_severity_names
 * @brief description of the RTAS severity levels
 */
static char *rtas_severity_names[] = {
    "No Error", "Event", "Warning", "Error Sync", "Error",
    "Fatal", "Already Reported", "(7)"
};

/**
 * rtas_disposition_names
 * @brief description of the RTAS event disposition levels
 */
static char *rtas_disposition_names[] = {
    "Fully Recovered", "Limited Recovery", "Not Recoverd", "(4)"
};

/**
 * rtas_entity_names
 * @brief description of the initiator and target names
 */
char *rtas_entity_names[] = { /* for initiator & targets */
    "Unknown", "CPU", "PCI", "ISA", "Memory", "Power Management",
    "Hot Plug", "(7)", "(8)", "(9)", "(10)", "(11)", "(12)",
    "(13)", "(14)", "(15)"
};

/**
 * rtas_error_type
 * @brief description of some of the RTAS error types
 *
 * Not all of the event types are covered in this array, please
 * bounds check before using.
 */
static char *rtas_event_error_type[] = {
    "Unknown", "Retry", "TCE Error", "Internal Device Failure",
    "Timeout", "Data Parity", "Address Parity", "Cache Parity",
    "Address Invalid", "ECC Uncorrected", "ECC Corrupted",
};

/**
 * print_scn_title
 * @brief print the title of the RTAS event section
 *
 * @param fmt string format for section title
 * @param ... additional args a la printf()
 * @return number of characters printed
 */
int
print_scn_title(char *fmt, ...)
{
    va_list ap;
    int rspace;
    char buf[1024];
    int i, len, offset;

    memset(buf, 0, sizeof(buf));

    offset = snprintf(buf, sizeof(buf), "==== ");

    va_start(ap, fmt);
    offset += vsnprintf(buf + offset, sizeof(buf) - offset, fmt, ap);
    va_end(ap);

    offset += snprintf(buf + offset, sizeof(buf) - offset, " ");

    rspace = (rtas_print_width - (strlen(buf) + 2 + 9));
    for (i = 0; i < rspace; i++)
        offset += snprintf(buf + offset, sizeof(buf) - offset, "=");

    offset += snprintf(buf + offset, sizeof(buf) - offset, "\n");

    len = rtas_print(buf);
    
    return len; 
}

/**
 * print_raw_data
 * @brief dump raw data 
 *
 * @param data pointer to data to dump
 * @param data_len length of data to dump
 * @return number of bytes written
 */ 
int
print_raw_data(char *data, int data_len)
{
    unsigned char *h, *a;
    unsigned char *end = (unsigned char *)data + data_len;
    unsigned int offset = 0;
    int i,j;
    int len = 0;

    /* make sure we're starting on a new line */
    if (line_offset != 0)
        len += rtas_print("\n");

    h = a = (unsigned char *)data;

    while (h < end) {
        /* print offset */
        len += fprintf(ostream, "0x%04x:  ", offset);
        offset += 16;

        /* print hex */
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (h < end)
                    len += fprintf(ostream, "%02x", *h++);
                else
                    len += fprintf(ostream, "  ");
            }
            len += fprintf(ostream, " ");
        }

        /* print ascii */
        len += fprintf(ostream, "    [");
        for (i = 0; i < 16; i++) {
            if (a <= end) {
                if ((*a >= ' ') && (*a <= '~'))
                    len += fprintf(ostream, "%c", *a);
                else
                    len += fprintf(ostream, ".");
                a++;
            } else
                len += fprintf(ostream, " ");

        }
        len += fprintf(ostream, "]\n");
    }

    return len;
}

/**
 * rtas_print_raw_event
 * @brief Dump the entire rtas event in raw format
 *
 * @param stream ouput stream to write to
 * @param re rtas_event pointer
 * @return number of bytes written
 */ 
int
rtas_print_raw_event(FILE *stream, struct rtas_event *re)
{
    int len = 0;
    
    ostream = stream;
    
    len += print_scn_title("Raw RTAS Event Begin");
    len += print_raw_data(re->buffer, re->event_length);
    len += print_scn_title("Raw RTAS Event End");

    return len;
}

/**
 * rtas_error_type
 * @brief print a description of the RTAS error type
 *
 * @param type RTAS event type
 * @return pointer to description of RTAS type
 * @return NULL on unrecognized event type
 */
static char *
rtas_error_type(int type)
{
    if (type < 11)
        return rtas_event_error_type[type];

    switch (type) {
        case 64:    return "EPOW";
	case 160:   return "Platform Resource Reassignment";
        case 224:   return "Platform Error";
        case 225:   return "I/O Event";
        case 226:   return "Platform Information Event";
        case 227:   return "Resource Deallocation Event";
        case 228:   return "Dump Notification Event";
    }

    return rtas_event_error_type[0];
}

/**
 * rtas_set_print_width
 * @brief set the output character width for librtasevent
 *
 * @param width character width of output
 * @return 0 on success, !0 on failure
 */
int
rtas_set_print_width(int width)
{
    if ((width > 0) && (width < 1024)) {
        rtas_print_width = width;
        return 0;
    }

    return 1;
}

/** 
 * rtas_print
 * @brief routine to handle all librtas printing
 *
 * @param fmt string format a la printf()
 * @param ... additional args a la printf()
 * @return number of bytes written
 */
int
rtas_print(char *fmt, ...)
{
    va_list     ap;
    char        buf[1024];
    char        tmpbuf[1024];
    int         i;
    int         buf_offset = 0, offset = 0;
    int         tmpbuf_len;
    int         width = 0;
    int         prnt_len;
    char        *newline = NULL;
    char        *brkpt = NULL;

    memset(tmpbuf, 0, sizeof(tmpbuf));
    memset(buf, 0, sizeof(buf));

    va_start(ap, fmt);
    tmpbuf_len = vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);
    va_end(ap);

    i = 0;
    while (i < tmpbuf_len) {

        brkpt = NULL;
        newline = NULL;

        for (i = offset, width = line_offset; 
             (width < rtas_print_width) && (i < tmpbuf_len); 
             i++) {
            
            switch(tmpbuf[i]) {
                case ' ':
                case '-':
                            width++;
                            brkpt = &tmpbuf[i];
                            break;

                case '\n':
                            newline = &tmpbuf[i];
                            width++;
                            break;
                                
                default:
                            width++;
                            break;
            }

            if (newline != NULL) {
                prnt_len = newline - &tmpbuf[offset] + 1;
                snprintf(buf + buf_offset, prnt_len, &tmpbuf[offset]);
                buf_offset = strlen(buf);
                buf_offset += snprintf(buf + buf_offset,
				       sizeof(buf) - buf_offset, "\n");
                offset += prnt_len;
                line_offset = 0;
                break;
            }
        }

        if (width >= rtas_print_width) {

            if (brkpt == NULL) {
               /* this won't fit on one line, break it across lines */
                prnt_len = width - line_offset + 1;
            } else {
                prnt_len = (brkpt - &tmpbuf[offset]) + 1;
            }

            /* print up to the last brkpt */
            snprintf(buf + buf_offset, prnt_len, &tmpbuf[offset]);
            buf_offset = strlen(buf);
            buf_offset += snprintf(buf + buf_offset, sizeof(buf) - buf_offset,
				   "\n");
            offset += prnt_len;
            line_offset = 0;
        }
            
    } 

    prnt_len = snprintf(buf + buf_offset, sizeof(buf) - buf_offset,
			&tmpbuf[offset]);
    line_offset += prnt_len;

    return fprintf(ostream, buf);
}

/** 
 * rtas_print_scn
 * @brief print the contents of the specified rtas event section
 *
 * @param stream output stream to write to
 * @param res rtas_event_scn pointer to print
 * @param verbosity verbose level for output
 * @return number of bytes written
 */
int
rtas_print_scn(FILE *stream, struct scn_header *shdr, int verbosity)
{
    int len;

    if ((stream == NULL) || (shdr == NULL)) {
        errno = EFAULT;
        return 0;
    }

    ostream = stream;

    /* validate the section id */
    if (shdr->scn_id < 0) {
        errno = EFAULT;
        return 0;
    }

    len = re_print_fns[shdr->scn_id](shdr, verbosity);

    fflush(stream);
    return len;
}

/**
 * rtas_print_event
 * @brief print the contents of an  entire rtas event
 *
 * @param stream output stream to print to
 * @param re rtas_event pointer to print out
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
rtas_print_event(FILE *stream, struct rtas_event *re, int verbosity)
{
    struct scn_header *shdr;
    int len = 0;

    if ((stream == NULL) || (re == NULL)) {
        errno = EFAULT;
        return 0;
    }

    ostream = stream;

    if (re->event_no != -1)
        len += print_scn_title("RTAS Event Dump (%d) Begin", re->event_no);
    else
        len += print_scn_title("RTAS Event Dump Begin");

    for (shdr = re->event_scns; shdr != NULL; shdr = shdr->next)
        len += rtas_print_scn(stream, shdr, verbosity);

    if (re->event_no != -1)
        len += print_scn_title("RTAS Event Dump (%d) End", re->event_no);
    else
        len += print_scn_title("RTAS Event Dump End");

    return len;
}

/**
 * rtas_get_event_hdr_scn
 * @brief Retrieve the Main RTAS event header
 *
 * @param re rtas_event pointer
 * @return pointer to rtas_event_scn for main rtas event header
 */
struct rtas_event_hdr *
rtas_get_event_hdr_scn(struct rtas_event *re)
{
    return (struct rtas_event_hdr *)get_re_scn(re, RTAS_EVENT_HDR);
}

/**
 * print_re_hdr_scn
 * @brief Print the contents of an RTAS main event header
 *
 * @param res rtas_event_scn pointer for main RTAS event header
 * @param verbosity verbose level for output
 * @return number of bytes written
 */
int
print_re_hdr_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_event_hdr *re_hdr;
    int len = 0;

    if (shdr->scn_id != RTAS_EVENT_HDR) {
        errno = EFAULT;
        return 0;
    }    

    re_hdr = (struct rtas_event_hdr *)shdr;

    len += rtas_print(PRNT_FMT"    ", "Version:", re_hdr->version);
    len += rtas_print(PRNT_FMT" (%s)\n", "Severity:", re_hdr->severity,
                      rtas_severity_names[re_hdr->severity]);

    if (re_hdr->disposition || (verbosity >= 2)) {
        len += rtas_print(PRNT_FMT" (%s)\n", "Disposition:", 
                          re_hdr->disposition, 
                          rtas_disposition_names[re_hdr->disposition]);
    }

    if (verbosity >= 2) {
        len += rtas_print(PRNT_FMT"    ", "Extended:", re_hdr->extended);
        len += rtas_print(PRNT_FMT"\n", "Log Length:", 
                          re_hdr->ext_log_length);
    }

    if (re_hdr->initiator || verbosity >=2) {
        len += rtas_print(PRNT_FMT" (%s)\n", "Initiator", re_hdr->initiator,
                          rtas_entity_names[re_hdr->initiator]);
    }

    if (re_hdr->target || (verbosity >= 2)) {
        len += rtas_print(PRNT_FMT" (%s)\n", "Target", re_hdr->target, 
                          rtas_entity_names[re_hdr->target]);
    }

    len += rtas_print(PRNT_FMT" (%s)\n", "Type", re_hdr->type, 
                      rtas_error_type(re_hdr->type));

    return len;
}

/**
 * rtas_get_event_exthdr_scn
 * @brief Retrieve the RTAS Event extended header
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for RTAS extended header section
 */
struct rtas_event_exthdr *
rtas_get_event_exthdr_scn(struct rtas_event *re)
{
    return (struct rtas_event_exthdr *)get_re_scn(re, RTAS_EVENT_EXT_HDR);
}

/**
 * print_re_exthdr_scn
 * @brief print the contents of the RTAS extended header section
 *
 * @param res rtas_event_scn pointer for the extended header
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_exthdr_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_event_exthdr *rex_hdr;
    int len = 0;
    int version;

    if (shdr->scn_id != RTAS_EVENT_EXT_HDR) {
        errno = EFAULT;
        return 0;
    }    

    rex_hdr = (struct rtas_event_exthdr *)shdr;
    version = shdr->re->version;

    if (!rex_hdr->valid) {
        if (rex_hdr->bigendian && rex_hdr->power_pc)
            len += rtas_print("Extended log data is not valid.\n\n");
        else
            len += rtas_print("Extended log data can not be decoded.\n\n");

        return len;
    }

    /* Dump useful stuff in the rex_hdr */
    len += rtas_print("%-19s%s%s%s%s%s\n", "Status:",
                      rex_hdr->unrecoverable ? " unrecoverable" : "",
                      rex_hdr->recoverable ? " recoverable" : "",
                      rex_hdr->unrecoverable_bypassed ? " bypassed" : "",
                      rex_hdr->predictive ? " predictive" : "",
                      rex_hdr->newlog ? " new" : "");

    if (version < 6) {
        if (version >= 3) {
            if (rex_hdr->non_hardware)
                len += rtas_print("Error may be caused by defects in " 
                                  "software or firmware.\n");
            if (rex_hdr->hot_plug)
                len += rtas_print("Error is isolated to hot-pluggable unit.\n");
            if (rex_hdr->group_failure)
                len += rtas_print("Error is isolated to a group of failing "
                                  "units.\n");
        }

        if (rex_hdr->residual)
            len += rtas_print("Residual error from previous boot.\n");
        if (rex_hdr->boot)
            len += rtas_print("Error detected during IPL process.\n");
        if (rex_hdr->config_change)
            len += rtas_print("Configuration changed since last boot.\n");
        if (rex_hdr->post)
            len += rtas_print("Error detected prior to IPL.\n");

	len += rtas_print("%-20s%x/%x/%x  %-20s%x:%x:%x:%x\n\n", "Date:",
			  rex_hdr->date.year, rex_hdr->date.month, 
			  rex_hdr->date.day, "Time:", rex_hdr->time.hour, 
			  rex_hdr->time.minutes, rex_hdr->time.seconds,
			  rex_hdr->time.hundredths);
    } 
    else {
        rtas_print("\n");
    }

    return len;
}
