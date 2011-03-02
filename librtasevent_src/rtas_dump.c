/**
 * @file rtas_dump.c
 * @brief RTAS version 6 Dump Locator section routines 
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
 * parse_rtas_dump_scn
 *
 */
int
parse_dump_scn(struct rtas_event *re)
{
    struct rtas_dump_scn *dump;

    dump = malloc(sizeof(*dump));
    if (dump == NULL) {
        errno = ENOMEM;
        return -1;
    }

    dump->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(dump), re, RE_V6_DUMP_SCN_SZ);
    add_re_scn(re, dump, RTAS_DUMP_SCN);

    return 0;
}

/**
 * rtas_get_dump_scn
 * @brief Retrieve the Dump Locator section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer to dump locator section
 */
struct rtas_dump_scn *
rtas_get_dump_scn(struct rtas_event *re)
{
    return (struct rtas_dump_scn *)get_re_scn(re, RTAS_DUMP_SCN);
}

/**
 * update_os_dump_id
 *
 */
int
update_os_id_scn(struct rtas_event *re, const char *id)
{
	struct rtas_dump_scn *dump_scn;
	struct rtas_dump_scn *raw_scn;
	int len;

	dump_scn = rtas_get_dump_scn(re);
	if (dump_scn == NULL)
		return -1;

	len = strlen(id);
	if (len > 40)
		return -1;

	/* Now we know it exist, get a pointer to the dump section
	 * in the raw event.
	 */
	raw_scn = (struct rtas_dump_scn *)(re->buffer 
					   + dump_scn->shdr.raw_offset 
					   - RE_SHDR_SZ);
	memcpy(raw_scn->os_id, id, len);

	if ((len % 4) > 0)
		len += (4 - (len % 4));

	raw_scn->id_len = len;

	return 0;
}

/**
 * print_v6_dump_scn
 * @brief Print the contents of a version 6 dump locator section
 *
 * @param res rtas_event_scn pointer for dump locator section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_dump_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_dump_scn *dump;
    int len = 0;

    if (shdr->scn_id != RTAS_DUMP_SCN) {
        errno = EFAULT;
        return 0;
    }

    dump = (struct rtas_dump_scn *)shdr;

    len += print_v6_hdr("Dump Locator section", &dump->v6hdr, verbosity);
    len += rtas_print(PRNT_FMT_L, "Dump ID:", dump->id);

    len += rtas_print("%-20s%8s\n", "Dump Field Format:", 
                      (dump->fname_type ? "hex" : "ascii"));
    len += rtas_print("%-20s%s\n", "Dump Location:", 
                      (dump->location ? "HMC" : "Partition"));
    len += rtas_print(PRNT_FMT_ADDR, "Dump Size:", 
                      dump->size_hi, dump->size_lo);

    if (verbosity >= 2) {
        len += rtas_print("%-20s%8s    ", "Dump Size Valid:", 
                          (dump->size_valid ? "Yes" : "No"));
        len += rtas_print(PRNT_FMT_R, "Dump ID Length:", dump->id_len);
 
        if (dump->id_len) {
            len += rtas_print("Dump ID:");
            if (dump->fname_type)
                len += print_raw_data(dump->os_id, dump->id_len);
            else
                len += rtas_print("%s\n", dump->os_id);
        }
    }

    len += rtas_print("\n");
    return len;
}
