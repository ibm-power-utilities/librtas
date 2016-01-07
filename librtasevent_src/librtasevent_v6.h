/**
 * @file librtasevent_v6.h
 * @brief Structure definitions for RTAS event version 6 sections
 *
 * Copyright (C) 2005 IBM Corporation.
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
 */

#ifndef _H_RE_V6_RTAS_EVENT
#define _H_RE_V6_RTAS_EVENT

#include <librtasevent.h>
#include <inttypes.h>

/**
 * @struct rtas_v6_scn_hdr
 * @brief common header for RTAS version 6 sections
 *
 * Almost every version 6 section starts with the same data as defined
 * in this structure.  This is provided to make further structure definitions 
 * and manipulation.
 */
struct rtas_v6_hdr {
    char        id[2];              /**< section id */
    uint32_t    length;             /**< section length */
    uint32_t    version;            /**< section version */
    uint32_t    subtype;            /**< section sub-type id */
    uint32_t    creator_comp_id;    /**< component id of section creator */
};

struct rtas_v6_hdr_raw {
    char        id[2];
    uint32_t    length:16;
    uint32_t    version:8;
    uint32_t    subtype:8;
    uint32_t    creator_comp_id:16;
}__attribute__((__packed__));

#define RTAS_V6_HDR_SIZE	16

/* defines for the rtas_event_scn id */
#define RTAS_DUMP_SCN_ID    "DH"
#define RTAS_EPOW_SCN_ID    "EP"
#define RTAS_HMC_SCN_ID     "HM"
#define RTAS_IO_SCN_ID      "IE"
#define RTAS_IP_SCN_ID      "LP"
#define RTAS_LRI_SCN_ID     "LR"
#define RTAS_MI_SCN_ID      "MI"
#define RTAS_MTMS_SCN_ID    "MT"
#define RTAS_PSRC_SCN_ID    "PS"
#define RTAS_SSRC_SCN_ID    "SS"
#define RTAS_SW_SCN_ID      "SW"
#define RTAS_UDD_SCN_ID     "UD"
#define RTAS_HP_SCN_ID      "HP"

/**
 * @struct rtas_v6_main_ascn_
 * @brief RTAS version 6 Main A Section
 */
struct rtas_priv_hdr_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;
    
    struct rtas_date date;
    struct rtas_time time;
 
    char	creator_id;         /**< subsystem creator id */
#define RTAS_PH_CREAT_SERVICE_PROC   'E'
#define RTAS_PH_CREAT_HYPERVISOR     'H'
#define RTAS_PH_CREAT_POWER_CONTROL  'W'
#define RTAS_PH_CREAT_PARTITION_FW   'L'

    uint32_t    scn_count;        /**< number of sections in log */
    uint32_t    creator_subid_hi;
    uint32_t    creator_subid_lo;

    uint32_t    plid;               /**< platform log id */
    uint32_t    log_entry_id;       /**< Unique log entry id */
    char        creator_subid_name[9];
};

struct rtas_priv_hdr_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;
    
    struct rtas_date_raw date;
    struct rtas_time_raw time;
 
    uint32_t    /* reserved */:32;
    uint32_t    /* reserved */:32;

    char	creator_id;
    uint32_t    /* reserved */ :16;
    uint32_t    scn_count:8;

    uint32_t    /* reserved */ :32;
    
    uint32_t    creator_subid_hi;
    uint32_t    creator_subid_lo;

    uint32_t    plid;
    uint32_t    log_entry_id;
    
    char        creator_subid_name[9];
}__attribute__((__packed__));

/**
 * @struct rtas_v6_main_b_scn
 * @brief RTAS version 6 Main B Section
 */
struct rtas_usr_hdr_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;
    
    uint32_t    subsystem_id;     /**< subsystem id */
    uint32_t    event_data;
    uint32_t    event_severity;
    uint32_t    event_type;       /**< error/event severity */
#define RTAS_UH_TYPE_NA                   0x00
#define RTAS_UH_TYPE_INFO_ONLY            0x01
#define RTAS_UH_TYPE_DUMP_NOTIFICATION    0x08
#define RTAS_UH_TYPE_PREVIOUSLY_REPORTED  0x10
#define RTAS_UH_TYPE_DECONFIG_USER        0x20
#define RTAS_UH_TYPE_DECONFIG_SYSTEM      0x21
#define RTAS_UH_TYPE_DECONFIG_NOTICE      0x22
#define RTAS_UH_TYPE_RETURN_TO_NORMAL     0x30
#define RTAS_UH_TYPE_CONCURRENT_MAINT     0x40
#define RTAS_UH_TYPE_CAPACITY UPGRADE     0x60
#define RTAS_UH_TYPE_RESOURCE_SPARING     0x70
#define RTAS_UH_TYPE_DYNAMIC_RECONFIG     0x80
#define RTAS_UH_TYPE_NORMAL_SHUTDOWN      0xD0
#define RTAS_UH_TYPE_ABNORMAL_SHUTDOWN    0xE0

    uint32_t    action;          /**< erro action code */
#define RTAS_UH_ACTION_SERVICE           0x8000
#define RTAS_UH_ACTION_HIDDEN            0x4000
#define RTAS_UH_ACTION_REPORT_EXTERNALLY 0x2000
#define RTAS_UH_ACTION_HMC_ONLY          0x1000
#define RTAS_UH_ACTION_CALL_HOME         0x0800
#define RTAS_UH_ACTION_ISO_INCOMPLETE    0x0400
};

struct rtas_usr_hdr_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;
    
    uint32_t    subsystem_id:8;
    uint32_t    event_data:8;
    uint32_t    event_severity:8;
    uint32_t    event_type:8;

    uint32_t    /* reserved */:32;

    uint32_t    /* reserved */:16;
    uint32_t	action:16;

    uint32_t    /* reserved */ :32;
}__attribute__((__packed__));

#define RE_USR_HDR_SCN_SZ     24

struct rtas_mtms {
    char        model[9];           /**< machine type / model */
    char        serial_no[13];      /**< serial number */
};

/**
 * @struct rtas_v6_dump_hdr
 * @brief RTAS version 6 dump locator section
 */
struct rtas_dump_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;
    /*These defines are for the v6hdr.subtype field in dump sections */
#define RTAS_DUMP_SUBTYPE_FSP       0x01
#define RTAS_DUMP_SUBTYPE_PLATFORM  0x02
#define RTAS_DUMP_SUBTYPE_SMA       0x03
#define RTAS_DUMP_SUBTYPE_POWER     0x04
#define RTAS_DUMP_SUBTYPE_LOG       0x05
#define RTAS_DUMP_SUBTYPE_PARTDUMP  0x06
#define RTAS_DUMP_SUBTYPE_PLATDUMP  0x07

    uint32_t    id;                  /**< dump id */
    uint32_t    location;            /**< 0 => dump sent to HMC
                                             1 => dump sent to partition */
    uint32_t    fname_type;          /**< 0 => file name in ASCII
                                             1 => file name in hex */
    uint32_t    size_valid;          /**< dump size field valid */
    
    uint32_t    id_len;              /**< OS assigned dump id length */
    
    uint32_t    size_hi;             /**< dump size (hi-bits) */
    uint32_t    size_lo;             /**< dump size (low bits) */
    char        os_id[40];           /**< OS assigned dump id */
};

struct rtas_dump_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;

    uint32_t    id;

    uint32_t	data1:8;
    
    uint32_t    /* reserved */ :16;
    uint32_t    id_len:8;

    uint32_t    size_hi;
    uint32_t    size_lo;
    char        os_id[40];
}__attribute__((__packed__));

#define RE_V6_DUMP_SCN_SZ	64


/**
 * @struct rtas_v6_lri_hdr
 * @brief RTAS v6 logical resource identification section
 */
struct rtas_lri_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;
    
    uint32_t    resource;             /**< resource type */
#define RTAS_LRI_RES_PROC           0x10
#define RTAS_LRI_RES_SHARED_PROC    0x11
#define RTAS_LRI_RES_MEM_PAGE       0x40
#define RTAS_LRI_RES_MEM_LMB        0x41

    uint32_t    capacity;            /**< entitled capacity */
    
    union {
        uint32_t _lri_cpu_id;        /**< logical CPU id (type = proc) */
	uint32_t _lri_drc_index;     /**< DRC index (type = mem LMB) */
	uint32_t _lri_mem_addr_lo;   /**< mem logical addr low bits
                                             (type = mem page) */
    } _lri_u1;
#define lri_cpu_id	_lri_u1._lri_cpu_id
#define	lri_drc_index	_lri_u1._lri_drc_index
#define lri_mem_addr_lo _lri_u1._lri_mem_addr_lo

    uint32_t    lri_mem_addr_hi;     /**< mem logical addr high bits
                                             (type = mem page) */
};

struct rtas_lri_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;
    
    uint32_t	resource:8;
    uint32_t	/* reserved */:8;
    uint32_t	capacity:16;

    union {
        uint32_t _lri_cpu_id:32;
        uint32_t _lri_drc_index:32;
        uint32_t _lri_mem_addr_lo;
    } _lri_u1;    
    
    uint32_t    lri_mem_addr_hi;
}__attribute__((__packed__));

#define RE_LRI_SCN_SZ   20

/**
 * @struct rtas_fru_hdr
 * @breif data that appears at the beginning of every FRU section
 */
struct rtas_fru_hdr {
    struct rtas_fru_hdr *next;
    char        id[2];
    uint32_t    length;
    uint32_t    flags;
};

struct rtas_fru_hdr_raw {
    char        id[2];
    uint32_t    length:8;
    uint32_t    flags:8;
}__attribute__((__packed__));

#define RE_FRU_HDR_SZ           4
#define RE_FRU_HDR_OFFSET(x)    ((char *)(x) + sizeof(struct rtas_fru_hdr *))

/**
 * @struct rtas_fru_id_scn
 * @breif Contents of the FRU Identity Substructure
 */
struct rtas_fru_id_scn {
    struct rtas_fru_hdr fruhdr;

#define RTAS_FRUID_COMP_MASK                0xF0
#define RTAS_FRUID_COMP_HARDWARE            0x10
#define RTAS_FRUID_COMP_CODE                0x20
#define RTAS_FRUID_COMP_CONFIG_ERROR        0x30
#define RTAS_FRUID_COMP_MAINT_REQUIRED      0x40
#define RTAS_FRUID_COMP_EXTERNAL            0x90
#define RTAS_FRUID_COMP_EXTERNAL_CODE       0xA0
#define RTAS_FRUID_COMP_TOOL                0xB0
#define RTAS_FRUID_COMP_SYMBOLIC            0xC0

#define RTAS_FRUID_HAS_PART_NO              0x08
#define RTAS_FRUID_HAS_CCIN                 0x04
#define RTAS_FRUID_HAS_PROC_ID              0x02
#define RTAS_FRUID_HAS_SERIAL_NO            0x01

#define fruid_has_part_no(x)    ((x)->fruhdr.flags & RTAS_FRUID_HAS_PART_NO)
#define fruid_has_ccin(x)       ((x)->fruhdr.flags & RTAS_FRUID_HAS_CCIN)
#define fruid_has_proc_id(x)    ((x)->fruhdr.flags & RTAS_FRUID_HAS_PROC_ID)
#define fruid_has_serial_no(x)  ((x)->fruhdr.flags & RTAS_FRUID_HAS_SERIAL_NO)

    char part_no[8];
    char procedure_id[8];
    char ccin[5];
    char serial_no[13];
};

struct rtas_fru_id_scn_raw {
    struct rtas_fru_hdr_raw fruhdr;

    /* The following fields may not be present */
    char part_no[8];
    char procedure_id[8];
    char ccin[5];
    char serial_no[13];
}__attribute__((__packed__));

/**
 * @struct rtas_fru_pe_scn
 * @brief contents of the FRU Power Enclosure Substructure
 */
struct rtas_fru_pe_scn {
    struct rtas_fru_hdr fruhdr;
    struct rtas_mtms pce_mtms;
    char pce_name[32];
};

struct rtas_fru_pe_scn_raw {
    struct rtas_fru_hdr_raw fruhdr;
    struct rtas_mtms pce_mtms;
    char pce_name[32];
}__attribute__((__packed__));

/**
 * @struct fru_mru
 * @brief FRU MR Description structs
 */
struct fru_mru {
    char        priority;
    uint32_t    id;
};

struct fru_mru_raw {
    uint32_t    /* reserved */:24;
    char        priority;
    uint32_t    id;
};

/**
 * @struct rtas_fru_mr_scn
 * @brief contents of the FRU Manufacturing Replacement Unit Substructure
 */
struct rtas_fru_mr_scn {
    struct rtas_fru_hdr fruhdr;

#define frumr_num_callouts(x)   ((x)->fruhdr.flags & 0x0F)

    uint32_t    /* reserved */:32;
    struct fru_mru  mrus[15];
};

struct rtas_fru_mr_scn_raw {
    struct rtas_fru_hdr_raw fruhdr;
    uint32_t    /* reserved */:32;
    struct fru_mru_raw  mrus[15];
};
    
/**
 * @struct rtas_v6_fru_scn
 * @brief RTAS version 6 FRU callout section
 */
struct rtas_fru_scn {
    uint32_t    length;               /**< call-out length */
    uint32_t    type;                 /**< callout type */
    uint32_t    fru_id_included;      /**< fru id subsection included */
    uint32_t    fru_subscn_included;

    char	priority;             /**< fru priority */
#define RTAS_FRU_PRIORITY_HIGH      'H'
#define RTAS_FRU_PRIORITY_MEDIUM    'M'
#define RTAS_FRU_PRIORITY_MEDIUM_A  'A'
#define RTAS_FRU_PRIORITY_MEDIUM_B  'B'
#define RTAS_FRU_PRIORITY_MEDIUM_C  'C'
#define RTAS_FRU_PRIORITY_LOW       'L'

    uint32_t    loc_code_length;      /**< location field length */
    char        loc_code[80];         /**< location code */
    struct rtas_fru_scn *next;
    struct rtas_fru_hdr *subscns;
};

struct rtas_fru_scn_raw {
    uint32_t    length:8;
    uint32_t	data1:8;
    char	priority;
    uint32_t    loc_code_length:8;
    char        loc_code[80];
};

#define RE_FRU_SCN_SZ       4

/**
 * @struct rtas_v6_src_hdr
 * @brief RTAS version 6 SRC section
 */
struct rtas_src_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;

    uint32_t    version;              /**< SRC version */ 
    char        src_platform_data[7]; /**< platform specific data */
#define src_subscns_included(src)    ((src)->src_platform_data[0] & 0x01)

    uint32_t    ext_refcode2;         /**< extended reference code word 2 */
    uint32_t    ext_refcode3;         /**< extended reference code word 3 */
    uint32_t    ext_refcode4;         /**< extended reference code word 4 */
    uint32_t    ext_refcode5;         /**< extended reference code word 5 */

    uint32_t    ext_refcode6;         /**< extended reference code word 6 */
    uint32_t    ext_refcode7;         /**< extended reference code word 7 */
    uint32_t    ext_refcode8;         /**< extended reference code word 8 */
    uint32_t    ext_refcode9;         /**< extended reference code word 9 */

    char        primary_refcode[36];  /**< primary reference code */
    
    uint32_t    subscn_id;	      /**< sub-section id (0xC0) */
    uint32_t    subscn_platform_data; /**< platform specific data */
    uint32_t    subscn_length;        /**< sub-section length */

    struct rtas_fru_scn *fru_scns;
};

struct rtas_src_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;

    uint32_t    version:8;
    char        src_platform_data[7];

    uint32_t    ext_refcode2:32;
    uint32_t    ext_refcode3:32;
    uint32_t    ext_refcode4:32;
    uint32_t    ext_refcode5:32;

    uint32_t    ext_refcode6:32;
    uint32_t    ext_refcode7:32;
    uint32_t    ext_refcode8:32;
    uint32_t    ext_refcode9:32;

    char        primary_refcode[36];
    
    uint32_t    subscn_id:8;
    uint32_t    subscn_platform_data:8;
    uint32_t    subscn_length:16;
};

#define RE_SRC_SCN_SZ       80
#define RE_SRC_SUBSCN_SZ    4

/**
 * @struct rtas_v6_mt_scn
 * @brief RTAS version 6 Machine Type section
 */
struct rtas_mt_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr_raw v6hdr;
    struct rtas_mtms mtms;
};

/**
 * @struct rtas_v6_generic
 */
struct rtas_v6_generic {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;
    char    *data;
};

/**
 * @struct rtas_hotplug_scn
 * @brief RTAS version 6 Hotplug section
 */
struct rtas_hotplug_scn {
    struct scn_header shdr;
    struct rtas_v6_hdr v6hdr;

    uint32_t    type:8;
#define RTAS_HP_TYPE_CPU	1
#define RTAS_HP_TYPE_MEMORY	2
#define RTAS_HP_TYPE_SLOT	3
#define RTAS_HP_TYPE_PHB	4
#define RTAS_HP_TYPE_PCI	5

    uint32_t	action:8;
#define RTAS_HP_ACTION_ADD	1
#define RTAS_HP_ACTION_REMOVE	2

    uint32_t    identifier:8;
#define RTAS_HP_ID_DRC_NAME	1
#define RTAS_HP_ID_DRC_INDEX	2
#define RTAS_HP_ID_DRC_COUNT	3

    uint32_t    reserved:8;
    union {
        uint32_t    drc_index:32;
        uint32_t    count:32;
        char        drc_name[1];
    } u1;
};

struct rtas_hotplug_scn_raw {
    struct rtas_v6_hdr_raw v6hdr;

    uint32_t    type:8;
    uint32_t    action:8;
    uint32_t    identifier:8;
    uint32_t    /* reserved */:8;

    union {
        uint32_t    drc_index:32;
        uint32_t    count:32;
        char        drc_name[1];
    } u1;
}__attribute__((__packed__));

#define RE_HOTPLUG_SCN_SZ	16

#endif 

