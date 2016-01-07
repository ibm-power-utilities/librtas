/**
 * @file rtas_sp.c
 * @brief RTAS Service Processor section routines
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
 * parse_sp_scn
 *
 */
int
parse_sp_scn(struct rtas_event *re)
{
    struct rtas_ibmsp_scn *sp;

    sp = malloc(sizeof(*sp));
    if (sp == NULL) {
        errno = ENOMEM;
        return 1;
    }

    sp->shdr.raw_offset = re->offset;

    rtas_copy(RE_SHDR_OFFSET(sp), re, RE_V4_SCN_SZ);
    add_re_scn(re, sp, RTAS_CPU_SCN);

    return 0;
}

/**
 * rtas_get_ibm_sp_scn
 * @brief Retrieve the IBM Service Processor Log section of the RTAS Event
 *
 * @param re rtas_event pointer
 * @return rtas_event_scn pointer for IBM SP section
 */
struct rtas_ibmsp_scn *
rtas_get_ibm_sp_scn(struct rtas_event *re)
{
    return (struct rtas_ibmsp_scn *)get_re_scn(re, RTAS_IBM_SP_SCN);
}

/**
 * print_re_ibmsp_scn
 * @brief print the contents of a RTAS Service Processor section
 *
 * @param res rtas_evnt_scn pointer for IBM SP section
 * @param verbosity verbose level of output
 * @return number of bytes written
 */
int
print_re_ibmsp_scn(struct scn_header *shdr, int verbosity)
{
    struct rtas_ibmsp_scn *sp;
    int len = 0;

    if (shdr->scn_id != RTAS_IBM_SP_SCN) {
        errno = EFAULT;
        return 0;
    }

    sp = (struct rtas_ibmsp_scn *)shdr;

    len += print_scn_title("Service Processor Section");

    if (strcmp(sp->ibm, "IBM") != 0)
	len += rtas_print("This log entry may be corrupt (IBM "
                          "signature malformed).\n");
    if (sp->timeout) 
	len += rtas_print("Timeout on communication response from "
                          "service processor.\n");
    if (sp->i2c_bus) 
	len += rtas_print("I2C general bus error.\n");
    if (sp->i2c_secondary_bus) 
	len += rtas_print("I2C secondary bus error.\n");
    
    if (sp->memory) 
	len += rtas_print("Internal service processor memory error.\n");
    if (sp->registers) 
	len += rtas_print("Service processor error accessing special "
                          "registers.\n");
    if (sp->communication) 
	len += rtas_print("Service processor reports unknown "
                          "communcation error.\n");
    if (sp->firmware) 
	len += rtas_print("Internal service processor firmware error.\n");
    
    if (sp->hardware) 
	len += rtas_print("Other internal service processor hardware "
                          "error.\n");
    if (sp->vpd_eeprom) 
	len += rtas_print("Service processor error accessing VPD EEPROM.\n");
    if (sp->op_panel) 
	len += rtas_print("Service processor error accessing Operator "
                          "Panel.\n");
    if (sp->power_controller) 
	len += rtas_print("Service processor error accessing Power "
                          "Controller.\n");
    
    if (sp->fan_sensor) 
	len += rtas_print("Service processor error accessing "
                          "Fan Sensor.\n");
    if (sp->thermal_sensor) 
	len += rtas_print("Service processor error accessing "
                          "Thermal Sensor.\n");
    if (sp->voltage_sensor) 
	len += rtas_print("Service processor error accessing "
                          "Voltage Sensor.\n");
    if (sp->serial_port) 
	len += rtas_print("Service processor error accessing "
                          "serial port.\n");
    
    if (sp->nvram) 
	len += rtas_print("Service processor detected NVRAM error.\n");
    if (sp->rtc) 
	len += rtas_print("Service processor error accessing real "
                          "time clock.\n");
    if (sp->jtag) 
	len += rtas_print("Service processor error accessing JTAG/COP.\n");
    if (sp->tod_battery) 
	len += rtas_print("Service processor or RTAS detects loss of "
                          "voltage \nfrom TOD battery.\n");
    
    if (sp->heartbeat) 
	len += rtas_print("Loss of heartbeat from Service processor.\n");
    if (sp->surveillance) 
	len += rtas_print("Service processor detected a surveillance "
                          "timeout.\n");
    if (sp->pcn_connection) 
	len += rtas_print("Power Control Network general connection "
                          "failure.\n");
    if (sp->pcn_node) 
	len += rtas_print("Power Control Network node failure.\n");
    
    if (sp->pcn_access) 
	len += rtas_print("Service processor error accessing Power "
                          "Control Network.\n");
    if (sp->sensor_token) 
	len += rtas_print(PRNT_FMT_R, "Sensor Token:", sp->sensor_token);
    if (sp->sensor_index) 
	len += rtas_print(PRNT_FMT_R, "Sensor Index:", sp->sensor_index);

    len += rtas_print("\n");
    return len;
}
