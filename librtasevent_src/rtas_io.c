/**
 * @file rtas_io.c
 * @brief RTAS I/O event section routines
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
 * parse_io_scn
 *
 */
int
parse_io_scn(struct rtas_event *re)
{
    struct rtas_io_scn *io;

    io = malloc(sizeof(*io));
    if (io == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(io, 0, sizeof(*io));
    io->shdr.raw_offset = re->offset;

    if (re->version < 6) {
        rtas_copy(RE_SHDR_OFFSET(io), re, RE_V4_SCN_SZ);
    } else {
        struct rtas_v6_io_scn_raw *rawhdr;

	rawhdr = (struct rtas_v6_io_scn_raw *)(re->buffer + re->offset);
	parse_v6_hdr(&io->v6hdr, &rawhdr->v6hdr);

	io->event_type = rawhdr->event_type; 
	io->rpc_length = rawhdr->rpc_length;
	io->scope = rawhdr->scope;
	io->subtype = rawhdr->subtype;

	io->drc_index = be32toh(rawhdr->drc_index);
	memcpy(io->rpc_data, rawhdr->rpc_data, 216);

	re->offset += io->v6hdr.length;
    }

    add_re_scn(re, io, RTAS_IO_SCN);

    return 0;
}

/**
 * rtas_get_io_scn
 * @brief Retrieve the I/O section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for i/o section
 */
struct rtas_io_scn *
rtas_get_io_scn(struct rtas_event *re)
{
    return (struct rtas_io_scn *)get_re_scn(re, RTAS_IO_SCN);
}

/**
 * print_v4_io
 * @brief print the contents of a RTAS pre-version 6 I/O section
 *
 * @param res rtas_event_scn pointer to i/o section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
static int
print_v4_io(struct scn_header *shdr, int verbosity)
{
    struct rtas_io_scn *io = (struct rtas_io_scn *)shdr;
    int len = 0;
 
    len += print_scn_title("I/O Event Section");

    if (io->bus_addr_parity) 
	len += rtas_print("I/O bus address parity.\n");
    if (io->bus_data_parity) 
	len += rtas_print("I/O bus data parity.\n");
    if (io->bus_timeout) 
	len += rtas_print("I/O bus timeout, access or other.\n");
    if (io->bridge_internal) 
	len += rtas_print("I/O bus bridge/device internal.\n");
    
    if (io->non_pci) 
	len += rtas_print("Signaling IOA is a PCI to non-PCI bridge " 
                          "(e.g. ISA).\n");
    if (io->mezzanine_addr_parity) 
	len += rtas_print("Mezzanine/System bus address parity.\n");
    if (io->mezzanine_data_parity) 
	len += rtas_print("Mezzanine/System bus data parity.\n");
    if (io->mezzanine_timeout) 
	len += rtas_print("Mezzanine/System bus timeout, transfer "
                          "or protocol.\n");
    
    if (io->bridge_via_sysbus) 
	len += rtas_print("Bridge is connected to system bus.\n");
    if (io->bridge_via_mezzanine) 
	len += rtas_print("Bridge is connected to memory controller "
                          "via mezzanine bus.\n");

    if (shdr->re->version >= 3) {
        if (io->bridge_via_expbus) 
	    len += rtas_print("Bridge is connected to I/O expansion bus.\n");
        if (io->detected_by_expbus) 
	    len += rtas_print("Error on system bus detected by I/O "
                              "expansion bus controller.\n");
        if (io->expbus_data_parity) 
	    len += rtas_print("I/O expansion bus data error.\n");
        if (io->expbus_timeout) 
	    len += rtas_print("I/O expansion bus timeout, access or other.\n");
        
        if (io->expbus_connection_failure) 
	    len += rtas_print("I/O expansion bus connection failure.\n");
        if (io->expbus_not_operating) 
	    len += rtas_print("I/O expansion unit not in an operating "
                           "state (powered off, off-line).\n");
    }

    len += rtas_print("IOA Signaling the error: %x:%x.%x\n    vendor: %04x  "
	              "device: %04x  rev: %02x  slot: %x\n", 
                      io->pci_sig_busno, io->pci_sig_devfn >> 3, 
                      io->pci_sig_devfn & 0x7U, io->pci_sig_vendorid, 
                      io->pci_sig_deviceid, io->pci_sig_revisionid, 
                      io->pci_sig_slot);
    
    len += rtas_print("IOA Sending during the error: %x:%x.%x\n"
                      "    vendor: %04x  device: %04x  rev: %02x  slot: %x\n", 
                      io->pci_send_busno, io->pci_send_devfn >> 3, 
                      io->pci_send_devfn & 0x7U, io->pci_send_vendorid, 
                      io->pci_send_deviceid, io->pci_send_revisionid, 
                      io->pci_send_slot);
   
    len += rtas_print("\n");
    return len;
}

/**
 * print_v6_io
 * @brief print the contents of a version 6 RTAS I/O section
 *
 * @param res rtas_event_scn pointer to i/o section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
static int
print_v6_io(struct scn_header *shdr, int verbosity)
{
    struct rtas_io_scn *io = (struct rtas_io_scn *)shdr;
    int len = 0;
    int has_rpc_data = 0;

    len += print_v6_hdr("I/O Event Section", &io->v6hdr, verbosity);
    
    len += rtas_print(PRNT_FMT_L, "Event Type:", io->event_type);
    switch (io->event_type) {
        case 0x01:
	    len += rtas_print(" - Error Detected.\n");
	    break;
            
	case 0x02:
	    len += rtas_print(" - Error Recovered.\n");
	    break;

	case 0x03:
	    len += rtas_print(" - Event (%x).\n", io->event_type);
	    break;
            
	case 0x04:
	    len += rtas_print(" - RPC Pass Through (%x).\n", io->event_type);
	    has_rpc_data = 1;
	    break;
                
	default:
	    len += rtas_print(" - Unknown event type (%x).\n", io->event_type);
	    break;
    }

    len += rtas_print(PRNT_FMT_L, "Error/Event Scope:", io->scope);
    switch (io->scope) {
        case 0x00:
	    len += rtas_print(" - N/A.\n");
	    break;
            
	case 0x36:
	    len += rtas_print(" - RIO-hub.\n");
	    break;
            
	case 0x37:
	    len += rtas_print(" - RIO-bridge.\n");
	    break;
            
	case 0x38:
	    len += rtas_print(" - PHB.\n");
	    break;
            
	case 0x39:
	    len += rtas_print(" - EADS Global.\n");
	    break;
            
	case 0x3A:
	    len += rtas_print(" - EADS Slot.\n");
	    break;
            
	default:
	    len += rtas_print(" - Unknown error/event scope.\n");
	    break;
    }

    len += rtas_print(PRNT_FMT_L, "I/O Event Subtype:", io->subtype);
    switch (io->subtype) {
        case 0x00:
	    len += rtas_print(" - N/A.\n");
	    break;
            
        case 0x01:
	    len += rtas_print(" - Rebalance Request.\n");
	    break;
            
        case 0x03:
	    len += rtas_print(" - Node online.\n");
	    break;
            
        case 0x04:
	    len += rtas_print(" - Node off-line.\n");
	    break;

	case 0x05:
	    len += rtas_print(" - Platform Dump maximum size change.\n");
	    break;
            
	default:
	    len += rtas_print(" - Unknown subtype.\n");
	    break;
    }

    len += rtas_print(PRNT_FMT_L, "DRC Index:", io->drc_index);

    if (has_rpc_data) {
        len += rtas_print(PRNT_FMT_R, "RPC Field Length:", io->rpc_length); 
        if (io->rpc_length != 0)
	    len += print_raw_data(io->rpc_data, io->rpc_length);
    } 
    else {
        len += rtas_print("\n");
    }

    return len;
}

/**
 * print_re_io_scn
 * @brief print the contents of a RTAS event i/o section
 *
 * @param res rtas_event_scn pointer for i/o section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */ 
int
print_re_io_scn(struct scn_header *shdr, int verbosity)
{
    if (shdr->scn_id != RTAS_IO_SCN) {
        errno = EFAULT;
        return 0;
    }

    if (shdr->re->version == 6)
        return print_v6_io(shdr, verbosity);
    else
        return print_v4_io(shdr, verbosity);
}

