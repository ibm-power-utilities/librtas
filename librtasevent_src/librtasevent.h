/**
 * @file librtasevent.h
 * @brief Structure and Interface definitions for librtasevent
 *
 * This provides the structure and interface definitions for librtasevent.
 * Also see librtasevent_v4.h and librtasevent_v6.h for additional
 * structure definitions particular those event versions.
 *
 * librtasevent usage:
 * The librtasevent library is meant to place the structure definitions,
 * parsing and printing of RTAS events into a common place.  The use of
 * librtasevent is easiest through the following interfaces:
 * 
 * parse_rtas_event() - This takes a buffer containing an RTAS event in
 *     binary form and returns a pointer to an rtas_event struct.  This
 *     struct has a list of rtas_event_scn objects hanging off of it
 *     for each section of the rtas event.
 * 
 * This presents the user with a broken down representation of the RTAS
 * event that can then be easily searched for any relevant information
 * or passed to either rtas_print_event() to print the entire RTAS event
 * or the rtas_print_event_scn() to print a particular section of the
 * RTAS event.
 *
 * When finished a call to rtas_cleanup_event() will free all of the data
 * structuires associated with the RTAS event.
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#ifndef _H_RTAS_EVENTS
#define _H_RTAS_EVENTS

#include <stdio.h>
#include <inttypes.h>

struct rtas_event;

/**
 * @struct rtas_event_scn
 * @brief Contains the data for a RTAS event section
 */
struct scn_header {
    struct scn_header   *next;
    struct rtas_event	*re;
    uint32_t		raw_offset;
    int                 scn_id;
};

#define RE_SHDR_SZ           (sizeof(struct scn_header))
#define RE_SHDR_OFFSET(x)    ((char *)(x) + RE_SHDR_SZ)

/* definitions for struct rtas_event_scn scn_id */
#define RTAS_EVENT_HDR		1
#define RTAS_EVENT_EXT_HDR	2

#define RTAS_EPOW_SCN		3
#define RTAS_IO_SCN		4

#define RTAS_CPU_SCN		5
#define RTAS_IBM_DIAG_SCN	6	
#define RTAS_MEM_SCN		7
#define RTAS_POST_SCN		8
#define RTAS_IBM_SP_SCN 	9
#define RTAS_VEND_ERRLOG_SCN    10

#define RTAS_PRIV_HDR_SCN       11
#define RTAS_USR_HDR_SCN	12
#define RTAS_DUMP_SCN		13	
#define RTAS_LRI_SCN		14
#define RTAS_MT_SCN		15
#define RTAS_PSRC_SCN		16
#define RTAS_SSRC_SCN		17
#define RTAS_GENERIC_SCN        18
#define RTAS_HP_SCN		19

#define RTAS_MAX_SCN_ID		20

/**
 * @struct rtas_event
 * @brief Anchor structure for parsed RTAS events
 */
struct rtas_event {
    int		version;
    int		event_no;
    char	*buffer;
    uint32_t	offset;
    uint32_t    event_length;
    struct scn_header *event_scns;
};

#define RE_EVENT_OFFSET(re)     ((re)->buffer + (re)->offset)

/**
 * @struct rtas_date
 * @brief definition of date format in rtas events
 */
struct rtas_date {
    uint32_t    year:16;
    uint32_t    month:8;
    uint32_t    day:8;
};

/**
 * @struct rtas_time
 * @brief definition of timestamp in rtas events
 */
struct rtas_time {
    uint32_t    hour:8;
    uint32_t    minutes:8;
    uint32_t    seconds:8;
    uint32_t    hundredths:8;
};

/* include version specific data */
#include "librtasevent_v4.h"
#include "librtasevent_v6.h"

/**
 * @struct rtas_event_hdr
 * @brief Fixed header at the beginning of all RTAS events
 */
struct rtas_event_hdr {
    struct scn_header shdr;
    uint32_t version:8;           /**< Architectural version */
    uint32_t severity:3;          /**< Severity level of error */
#define RTAS_HDR_SEV_NO_ERROR           0
#define RTAS_HDR_SEV_EVENT              1
#define RTAS_HDR_SEV_WARNING            2
#define RTAS_HDR_SEV_ERROR_SYNC         3
#define RTAS_HDR_SEV_ERROR              4
#define RTAS_HDR_SEV_FATAL              5
#define RTAS_HDR_SEV_ALREADY_REPORTED   6

    uint32_t disposition:2;       /**< Degree of recovery */
#define RTAS_HDR_DISP_FULLY_RECOVERED   0
#define RTAS_HDR_DISP_LIMITED_RECOVERY  1
#define RTAS_HDR_DISP_NOT_RECOVERED     2
    
    uint32_t extended:1;          /**< extended log present? */
    uint32_t /* reserved */ :2;
    uint32_t initiator:4;         /**< Initiator of event */
#define RTAS_HDR_INIT_UNKNOWN           0
#define RTAS_HDR_INIT_CPU               1
#define RTAS_HDR_INIT_PCI               2
#define RTAS_HDR_INIT_ISA               3
#define RTAS_HDR_INIT_MEMORY            4
#define RTAS_HDR_INIT_HOT_PLUG          5

    uint32_t target:4;            /**< Target of failed operation */
#define RTAS_HDR_TARGET_UNKNOWN         0
#define RTAS_HDR_TARGET_CPU             1
#define RTAS_HDR_TARGET_PCI             2
#define RTAS_HDR_TARGET_ISA             3
#define RTAS_HDR_TARGET_MEMORY          4
#define RTAS_HDR_TARGET_HOT_PLUG        5

    uint32_t type:8;              /**< General event or error*/
#define RTAS_HDR_TYPE_RETRY             1
#define RTAS_HDR_TYPE_TCE_ERR           2
#define RTAS_HDR_TYPE_INTERN_DEV_FAIL   3
#define RTAS_HDR_TYPE_TIMEOUT           4
#define RTAS_HDR_TYPE_DATA_PARITY       5
#define RTAS_HDR_TYPE_ADDR_PARITY       6
#define RTAS_HDR_TYPE_CACHE_PARITY      7
#define RTAS_HDR_TYPE_ADDR_INVALID      8
#define RTAS_HDR_TYPE_ECC_UNCORRECTED   9
#define RTAS_HDR_TYPE_ECC_CORRECTED     10 
#define RTAS_HDR_TYPE_EPOW              64 
#define RTAS_HDR_TYPE_PRRN		160
#define RTAS_HDR_TYPE_PLATFORM_ERROR    224
#define RTAS_HDR_TYPE_IBM_IO_EVENT      225
#define RTAS_HDR_TYPE_PLATFORM_INFO     226
#define RTAS_HDR_TYPE_RESOURCE_DEALLOC  227
#define RTAS_HDR_TYPE_DUMP_NOTIFICATION 228
#define RTAS_HDR_TYPE_HOTPLUG		229

    uint32_t ext_log_length:32;   /**< length in bytes */
};

#define RE_EVENT_HDR_SZ    8

/**
 * @struct rtas_event_exthdr
 * @brief RTAS optional extended error log header (12 bytes)
 */
struct rtas_event_exthdr {
    struct scn_header shdr;
    uint32_t valid:1;
    uint32_t unrecoverable:1;
    uint32_t recoverable:1;
    uint32_t unrecoverable_bypassed:1;  
    uint32_t predictive:1;
    uint32_t newlog:1;
    uint32_t bigendian:1; 
    uint32_t /* reserved */:1;

    uint32_t platform_specific:1;       /**< only in version 3+ */
    uint32_t /* reserved */:3;
    uint32_t platform_value:4;          /**< valid iff platform_specific */

    uint32_t power_pc:1;               
    uint32_t /* reserved */:2;
    uint32_t addr_invalid:1;     
    uint32_t format_type:4;
#define RTAS_EXTHDR_FMT_CPU             1
#define RTAS_EXTHDR_FMT_MEMORY          2
#define RTAS_EXTHDR_FMT_IO              3
#define RTAS_EXTHDR_FMT_POST            4
#define RTAS_EXTHDR_FMT_EPOW            5
#define RTAS_EXTHDR_FMT_IBM_DIAG        12
#define RTAS_EXTHDR_FMT_IBM_SP          13
#define RTAS_EXTHDR_FMT_VEND_SPECIFIC_1 14
#define RTAS_EXTHDR_FMT_VEND_SPECIFIC_2 15

    /* This group is in version 3+ only */
    uint32_t non_hardware:1;     /**<  Firmware or software is suspect */
    uint32_t hot_plug:1;         
    uint32_t group_failure:1;    
    uint32_t /* reserved */:1;

    uint32_t residual:1;         /**< Residual error from previous boot */
    uint32_t boot:1;             /**< Error during boot */
    uint32_t config_change:1;    /**< Configuration chang since last boot */
    uint32_t post:1;

    struct rtas_time time;       /**< Time of error in BCD HHMMSS00 */
    struct rtas_date date;       /**< Time of error in BCD YYYYMMDD */
};

#define RE_EXT_HDR_SZ   12

/**
 * @struct rtas_epow_scn 
 * @brief Common RTAS EPOW section
 */
struct rtas_epow_scn {
    struct scn_header shdr;

    /* The following represent the fields of a pre-version 6 RTAS EPOW event */
    uint32_t    sensor_value:28;     /**< EPOW sensor value */
    uint32_t    action_code:4;      /**< EPOW action code */
#define RTAS_EPOW_ACTION_RESET              0x00
#define RTAS_EPOW_ACTION_WARN_COOLING       0x01
#define RTAS_EPOW_ACTION_WARN_POWER         0x02
#define RTAS_EPOW_ACTION_SYSTEM_SHUTDOWN    0x03
#define RTAS_EPOW_ACTION_SYSTEM_HALT        0x04
#define RTAS_EPOW_ACTION_MAIN_ENCLOSURE     0x05
#define RTAS_EPOW_ACTION_POWER_OFF          0x07

    uint32_t    sensor:1;          /**< detected by a defined sensor */
    uint32_t    power_fault:1;     /**< caused by a power fault */
    uint32_t    fan:1;             /**< caused by a fan failure */

    uint32_t    temp:1;            /**< caused by over-temperature condition */
    uint32_t    redundancy:1;      /**< loss of redundancy */
    uint32_t    CUoD:1;            /**< CUoD entitlement exceeded */
    uint32_t    /* reserved */ :2;

    uint32_t    general:1;         /**< general unspecified fault */
    uint32_t    power_loss:1;      /**< fault due to loss of power source */
    uint32_t    power_supply:1;    /**< fault due to internal power failure */
    uint32_t    power_switch:1;    /**< manual activation of power-off switch */
    
    uint32_t    battery:1;         /**< fault due to internal battery failure */
    uint32_t    /* reserved */ :3;
    uint32_t    /* reserved */ :16;
    uint32_t    sensor_token:32;   /**< token for sensor causing the EPOW */

    uint32_t    sensor_index:32;   /**< index number of sensor causing EPOW */
    uint32_t    sensor_value2:32;  
    uint32_t    sensor_status:32;
    uint32_t    /* reserved */ :32;

    /* The following represent a version 6 RTAS EPOW event */
    struct rtas_v6_hdr v6hdr;
 
    uint32_t    _v6_sensor_value:4;
    uint32_t    _v6_action_code:4;
    uint32_t    event_modifier:8;       /**< EPOW event modifier */
#define RTAS_EPOW_MOD_NA                    0x00
#define RTAS_EPOW_MOD_NORMAL_SHUTDOWN       0x01
#define RTAS_EPOW_MOD_UTILITY_POWER_LOSS    0x02
#define RTAS_EPOW_MOD_CRIT_FUNC_LOSS        0x03
#define RTAS_EPOW_MOD_AMBIENT_TEMP          0x04

    uint32_t    /* reserved */ :16;
    char        reason_code[8];         /**< platform specific reason code */
};

/* defines used for copying in data from the RTAS event */
#define RE_EPOW_V6_SCN_SZ   20

/**
 * @struct rtas_io_scn
 * @brief RTAS i/o section
 */
struct rtas_io_scn {
    struct scn_header shdr;

    /* The following represents the pre-version 6 RTAS event */
    uint32_t    bus_addr_parity:1;        /**< bus address parity error */
    uint32_t    bus_data_parity:1;        /**< bus data pariy error */
    uint32_t    bus_timeout:1;            /**< bus time-out */
    uint32_t    bridge_internal:1;        /**< bridge/device internal error */

    uint32_t    non_pci:1;                /**< i.e. secondary bus such as ISA */
    uint32_t    mezzanine_addr_parity:1;  /**< Mezzanine/System address parity*/
    uint32_t    mezzanine_data_parity:1;  /**< Mezzanine/System data parity */
    uint32_t    mezzanine_timeout:1;      /**< Mezzanine/System bus time-out */

    uint32_t    bridge_via_sysbus:1;      /**< bridge connected to System bus */
    uint32_t    bridge_via_mezzanine:1;   /**< bridge is connected to memory 
                                               controller via Mezzanine Bus */
    uint32_t    bridge_via_expbus:1;      /**< bridge is connected to I/O
                                               expansion bus */
    uint32_t    detected_by_expbus:1;     /**< detected by I/O Expansion bus */
    
    uint32_t    expbus_data_parity:1;     /**< expansion bus (parity, CRC) 
                                               error */
    uint32_t    expbus_timeout:1;         /**< expansion bus time-out */
    uint32_t    expbus_connection_failure:1; 
    uint32_t    expbus_not_operating:1;   /**< expansion bus not in an 
                                               operating state */

    /* PCI Bus Data of the IOA signalling the error */
    uint32_t    pci_sig_bus_id:8;         /**< IOA signalling the error */
    uint32_t    pci_sig_busno:8;          /**< PCI device ID */
    uint32_t    pci_sig_devfn:8;          /**< PCI function ID */
    uint32_t    pci_sig_deviceid:16;      /**< PCI "Device ID"
                                               from configuration register */
    uint32_t    pci_sig_vendorid:16;      /**< PCI "Vendor ID"
                                               from configuration register */
    
    uint32_t    pci_sig_revisionid:8;     /**< PCI "Revision ID" 
                                               from configuration register */
    uint32_t    pci_sig_slot:8;           /**< Slot identifier number
                                               00=>system board, ff=>multiple */

    /* PCI Bus Data of the IOA sending the the error */
    uint32_t    pci_send_bus_id:8;        /**< IOA sending at time of error */
    uint32_t    pci_send_busno:8;         /**< PCI device ID */
    uint32_t    pci_send_devfn:8;         /**< PCI function ID */
    uint32_t    pci_send_deviceid:16;     /**< PCI "Device ID"
                                               from configuration register */
    uint32_t    pci_send_vendorid:16;     /**< PCI "Vendor ID"
                                               from configuration register */

    uint32_t    pci_send_revisionid:8;    /**< PCI "REvision ID"
                                               from configuration register */
    uint32_t    pci_send_slot:8;          /**< Slot identifier number
                                               00=>system board, ff=>multiple */

    uint32_t    /* reserved */ :16;
    uint32_t    /* reserved */ :32;
    uint32_t    /* reserved */ :32;

    /* The following represents the version 6 rtas event */
    struct rtas_v6_hdr v6hdr;
   
    uint32_t    event_type:8;           /**< I/O event type */
#define RTAS_IO_TYPE_DETECTED       0x01
#define RTAS_IO_TYPE_RECOVERED      0x02
#define RTAS_IO_TYPE_EVENT          0x03
#define RTAS_IO_TYPE_RPC_PASS_THRU  0x04

    uint32_t    rpc_length:8;           /**< RPC field length.  The RPC data
                                             is optional and appears after
                                             this structure in the event if
                                             present */
    uint32_t    scope:8;                /**< event scope */
#define RTAS_IO_SCOPE_NA            0x00
#define RTAS_IO_SCOPE_RIO_HUB       0x36
#define RTAS_IO_SCOPE_RIO_BRIDGE    0x37
#define RTAS_IO_SCOPE_PHB           0x38
#define RTAS_IO_SCOPE_EADS_GLOBAL   0x39
#define RTAS_IO_SCOPE_EADS_SLOT     0x3A
    
    uint32_t    subtype:8;              /**< I/O event sub-type */
#define RTAS_IO_SUBTYPE_NA              0x00
#define RTAS_IO_SUBTYPE_REBALANCE       0x01
#define RTAS_IO_SUBTYPE_NODE_ONLINE     0x02
#define RTAS_IO_SUBTYPE_NODE_OFFLINE    0x04
#define RTAS_IO_SUBTYPE_PLAT_DUMP_SZ	0x05

    uint32_t    drc_index:32;           /**< DRC index */
    char        rpc_data[216];
};
    
#define RE_IO_V6_SCN_OFFSET     (RE_SCN_HDR_SZ + RE_V4_SCN_SZ)

/* Retrieving and free'ing parsed RTAS events */ 
struct rtas_event * parse_rtas_event(char *, int);
int cleanup_rtas_event(struct rtas_event *);

/* Retrieving a particular section from a parsed RTAS event */
struct rtas_event_hdr * rtas_get_event_hdr_scn(struct rtas_event *);
struct rtas_event_exthdr * rtas_get_event_exthdr_scn(struct rtas_event *);

struct rtas_epow_scn * rtas_get_epow_scn(struct rtas_event *);
struct rtas_io_scn * rtas_get_io_scn(struct rtas_event *);

struct rtas_cpu_scn * rtas_get_cpu_scn(struct rtas_event *);
struct rtas_ibm_diag_scn * rtas_get_ibmdiag_scn(struct rtas_event *);
struct rtas_mem_scn * rtas_get_mem_scn(struct rtas_event *);
struct rtas_post_scn * rtas_get_post_scn(struct rtas_event *);
struct rtas_ibmsp_scn * rtas_get_ibm_sp_scn(struct rtas_event *);
struct rtas_vend_errlog_scn * rtas_get_vend_errlog_scn(struct rtas_event *);

struct rtas_priv_hdr_scn * rtas_get_priv_hdr_scn(struct rtas_event *);
struct rtas_usr_hdr_scn * rtas_get_usr_hdr_scn(struct rtas_event *);
struct rtas_dump_scn * rtas_get_dump_scn(struct rtas_event *);
struct rtas_lri_scn * rtas_get_lri_scn(struct rtas_event *);
struct rtas_mt_scn * rtas_get_mt_scn(struct rtas_event *);
struct rtas_src_scn * rtas_get_src_scn(struct rtas_event *);

struct rtas_hotplug_scn * rtas_get_hotplug_scn(struct rtas_event *);

int update_os_id_scn(struct rtas_event *, const char *);

/* Printing RTAS event data */
int rtas_print_scn(FILE *, struct scn_header *, int);
int rtas_print_event(FILE *, struct rtas_event *, int); 
int rtas_print_raw_event(FILE *, struct rtas_event *);
int rtas_set_print_width(int);

#endif
