/**
 * @file rtas_hotplug.c
 * @brief RTAS version 6 Hotplug section routines
 *
 * Copyright (C) 2013 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Tyrel Datwyler <tyreld@linux.vnet.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * parse_hotplug_scn
 *
 */
int
parse_hotplug_scn(struct rtas_event *re)
{
    struct rtas_hotplug_scn *hotplug;

    hotplug = malloc(sizeof(*hotplug));
    if (hotplug == NULL) {
        errno = ENOMEM;
        return -1;
    }

    hotplug->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(hotplug), re, RE_HOTPLUG_SCN_SZ);
    add_re_scn(re, hotplug, RTAS_HP_SCN);

    /* TODO: Fixup scn size when drc_name is included */

    return 0;
}

/**
 * rtas_get_hotplug_scn
 * @brief Retrieve the Hotplug section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer to Hotplug section
 */
struct rtas_hotplug_scn *
rtas_get_hotplug_scn(struct rtas_event *re)
{
    return (struct rtas_hotplug_scn *)get_re_scn(re, RTAS_HP_SCN);
}

/**
 * print_re_hotplug_scn
 * @brief Print the contents of a version 6 Hotplug section
 *
 * @param res rtas_event_scn pointer for Hotplug section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_hotplug_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_hotplug_scn *hotplug;
    int len = 0;

    if (shdr->scn_id != RTAS_HP_SCN) {
        errno = EFAULT;
        return 0;
    }

    hotplug = (struct rtas_hotplug_scn *)shdr;

    len += print_v6_hdr("Hotplug section", &hotplug->v6hdr, verbosity);
    len += rtas_print(PRNT_FMT_R, "Hotplug Type:", hotplug->type);

    len += rtas_print(PRNT_FMT_R, "Hotplug Action:", hotplug->action);

    len += rtas_print(PRNT_FMT_R, "Hotplug Identifier:", hotplug->identifier);

    if (hotplug->identifier == RTAS_HP_ID_DRC_NAME) {
	len += rtas_print("%-20s%s", "Hotplug drc_name:", hotplug->u1.drc_name);
    } else if (hotplug->identifier == RTAS_HP_ID_DRC_INDEX) {
	len += rtas_print(PRNT_FMT_R, "Hotplug drc_index:", hotplug->u1.drc_index);
    } else {
	len += rtas_print(PRNT_FMT_R, "Hotplug count:", hotplug->u1.count);
    }

    len += rtas_print("\n");
    return len;
}
