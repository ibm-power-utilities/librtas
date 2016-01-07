/**
 * @file rtas_post.c
 * @brief RTAS Power-On Self Test (POST) section routines
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
 * @author Jake Moilanen <moilanen@asutin.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "librtasevent.h"
#include "rtas_event.h"

/**
 * parse_post_scn
 *
 */
int
parse_post_scn(struct rtas_event *re)
{
    struct rtas_post_scn *post;

    post = malloc(sizeof(*post));
    if (post == NULL) {
        errno = ENOMEM;
        return -1;
    }

    post->shdr.raw_offset = re->offset;

    /* This memcpy will get us up through the devname... */
    rtas_copy(RE_SHDR_OFFSET(post), re, 14);
    post->devname[12] = '\0';
    
    /* Copy in the error code... */
    rtas_copy(post->err_code, re, 4);
    post->err_code[4] = '\0';

    /* Next the firmware revision level... */
    rtas_copy(post->firmware_rev, re, 2);
    post->firmware_rev[2] = '\0';

    /* and lastly, the location code */
    rtas_copy(post->loc_code, re, 8);
    post->loc_code[8] = '\0';

    add_re_scn(re, post, RTAS_CPU_SCN);

    return 0;
}

/**
 * rtas_get_post_ecn
 * @brief Retrieve the Power-On Self Test (POST) section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for post section
 */ 
struct rtas_post_scn *
rtas_get_post_scn(struct rtas_event *re)
{
    return (struct rtas_post_scn *)get_re_scn(re, RTAS_POST_SCN);
}

/**
 * print_re_post_scn
 * @brief print the contents of a POST section
 *
 * @param res rtas_event_scn pointer for post section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int 
print_re_post_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_post_scn *post = (struct rtas_post_scn *)shdr;
    int len = 0;
   
    if (shdr->scn_id != RTAS_POST_SCN) {
        errno = EFAULT;
        return 0;
    }

    len += print_scn_title("Power-On Self Test Section");

    if (post->devname[0]) 
	len += rtas_print("%-20s%s\n", "Failing Device:", post->devname);
    if (post->firmware) 
	len += rtas_print("Firmware Error.\n");
    if (post->config) 
	len += rtas_print("Configuration Error.\n");
    if (post->cpu) 
	len += rtas_print("CPU POST Error.\n");
    
    if (post->memory) 
	len += rtas_print("Memory POST Error.\n");
    if (post->io) 
	len += rtas_print("I/O Subsystem POST Error.\n");
    if (post->keyboard) 
	len += rtas_print("Keyboard POST Error.\n");
    if (post->mouse) 
	len += rtas_print("Mouse POST Error.\n");
    if (post->display) 
	len += rtas_print("Display POST Error.\n");

    if (post->ipl_floppy) 
	len += rtas_print("Floppy IPL Error.\n");
    if (post->ipl_controller) 
	len += rtas_print("Drive Controller Error during IPL.\n");
    if (post->ipl_cdrom) 
	len += rtas_print("CDROM IPL Error.\n");
    if (post->ipl_disk) 
	len += rtas_print("Disk IPL Error.\n");
    
    if (post->ipl_net) 
	len += rtas_print("Network IPL Error.\n");
    if (post->ipl_other) 
	len += rtas_print("Other (tape,flash) IPL Error.\n");
    if (post->firmware_selftest) 
	len += rtas_print("Self-test error in firmware extended "
                          "diagnostics.\n");

    len += rtas_print("POST Error Code:        %x\n", post->err_code);
    len += rtas_print("Firmware Revision Code: %x\n", post->firmware_rev);

    len += rtas_print("\n");
    return len;
}
