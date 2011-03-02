/**
 * @file librtasevent_v4.h
 * @brief Structure definitions for RTAS events pre-version 6
 *
 * Copyright (C) IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Nathan Fontenot <nfont@austin.ibm.com>
 */

#ifndef _H_RTAS_EVENT_HDRS
#define _H_RTAS_EVENT_HDRS

#include <librtasevent.h>
#include <inttypes.h>

#define RE_V4_SCN_SZ	28

/**
 * @struct rtas_cpu_scn
 * @brief RTAS CPU section
 */
struct rtas_cpu_scn {
    struct scn_header shdr;
    uint32_t    internal:1;         /**< internal error, other than cache */
    uint32_t    intcache:1;         /**< internal cache error */
    uint32_t    extcache_parity:1;  /**< or multi-bit ECC */
    uint32_t    extcache_ecc:1;     /**< single bit ecc error */

    uint32_t    sysbus_timeout:1;   /**< timeout waiting for mem controller*/
    uint32_t    io_timeout:1;       /**< timeout waiting for I/O */
    uint32_t    sysbus_parity:1;    /**< Address/Data parity error */
    uint32_t    sysbus_protocol:1;  /**< transfer error */
    
    uint32_t    id:8;               /**< physical CPU id number */
    uint32_t    element:16;         /**< id number of sender of parity error
                                         or element which timed out */
    uint32_t    failing_address_hi:32;
    uint32_t    failing_address_lo:32;
    uint32_t    try_reboot:1;       /**< V4: 1 => reboot may fix fault */

    uint32_t    /* reserved */ :7;
    uint32_t    /* reserved */ :24;
    uint32_t    /* reserved */ :32;
    uint32_t    /* reserved */ :32;
    uint32_t    /* reserved */ :32;
};

/**
 * @struct rtas_mem_scn
 * @brief RTAS memory controller section
 */
struct rtas_mem_scn {
    struct scn_header shdr;
    uint32_t    uncorrectable:1;        /**< uncorrectable memory error */
    uint32_t    ECC:1;                  /**< ECC correctable error */
    uint32_t    threshold_exceeded:1;   /**< correctable error threshold
                                             exceeded */
    uint32_t    control_internal:1;     /**< memory controller internal error */
    
    uint32_t    bad_address:1;          /**< memory address error */
    uint32_t    bad_data:1;             /**< memort data error */
    uint32_t    bus:1;                  /**< memory bus/switch internal error */
    uint32_t    timeout:1;              /**< memory time-out error */

    uint32_t    sysbus_parity:1;        /**< system bus parity error */
    uint32_t    sysbus_timeout:1;       /**< system bus time-out error */
    uint32_t    sysbus_protocol:1;      /**< system bus protocol/transfer 
                                             error */
    
    uint32_t    hostbridge_timeout:1;   /**< host bridge time-out error */
    uint32_t    hostbridge_parity:1;    /**< host bridge address/data parity 
                                             error */
    uint32_t    /* reserved */:1;
    uint32_t    support:1;              /**< system support function error */
    uint32_t    sysbus_internal:1;      /**< system bus internal 
                                             hardware/switch error */

    uint32_t    controller_detected:8;  /**< physical memory controller number 
                                             that detected the error */
    uint32_t    controller_faulted:8;   /**< physical memory controller number
                                             that caused the error */

    uint32_t    failing_address_hi:32;
    uint32_t    failing_address_lo:32;

    uint32_t    ecc_syndrome:16;        /**< syndrome bits */
    uint32_t    memory_card:8;          /**< memory card number */
    uint32_t    /* reserved */:8;
    uint32_t    sub_elements:32;        /**< memory sub-elements implicated
                                             on this card */
    uint32_t    element:16;             /**< id number of sender of address/data
                                             parity error */

    uint32_t    /* reserved */:16;
    uint32_t    /* reserved */:32;
};

/**
 * struct rtas_post_scn
 * @brief RTAS power-on self test section
 */
struct rtas_post_scn {
    struct scn_header shdr;
    uint32_t    firmware:1;             /**< firmware error */
    uint32_t    config:1;               /**< configuration error */
    uint32_t    cpu:1;                  /**< CPU POST error */
    uint32_t    memory:1;               /**< memory POST error */
   
    uint32_t    io:1;                   /**< I/O subsystem POST error */
    uint32_t    keyboard:1;             /**< keyboard POST error */
    uint32_t    mouse:1;                /**< mouse POST error */
    uint32_t    display:1;              /**< Graphic IOA / Display POST error */

    uint32_t    ipl_floppy:1;           /**< diskette IPL error */
    uint32_t    ipl_controller:1;       /**< drive controller IPL error */
    uint32_t    ipl_cdrom:1;            /**< cd-rom IPL error */
    uint32_t    ipl_disk:1;             /**< hard disk IPL error */
 
    uint32_t    ipl_net:1;              /**< network IPL error */
    uint32_t    ipl_other:1;            /**< other device IPL error */
    uint32_t    /* reserved */ :1;
    uint32_t    firmware_selftest:1;    /**< sef-test error in firmware */

    char        devname[13];            /**< device name */
    uint32_t    err_code[5];            /**< POST error code */
    uint32_t    firmware_rev[3];        /**< firmware revision level */
    uint32_t    loc_code[9];            /**< location code of failing device */
};

/**
 * @struct rtas_ibmsp_scn
 * @brief RTAS event service processor section
 */
struct rtas_ibmsp_scn {
    struct scn_header shdr;
    char        ibm[4];		        /**< "IBM\0" */
    
    uint32_t    timeout:1;              /**< service processor timeout */
    uint32_t    i2c_bus:1;              /**< I/O (I2C) general bus error */
    uint32_t    i2c_secondary_bus:1;    /**< secondary I/O (I2C) bus error */
    uint32_t    memory:1;               /**< internal service processor 
                                             memory error */
   
    uint32_t    registers:1;            /**< erro accessing special registers */
    uint32_t    communication:1;        /**< unknown communication error */
    uint32_t    firmware:1;             /**< service processor firmware error
                                             or incorrect version */
    uint32_t    hardware:1;             /**< other internal service processor
                                             hardware error */

    uint32_t    vpd_eeprom:1;           /**< error accessing VPD EEPROM */
    uint32_t    op_panel:1;             /**< error accessing operator panel */
    uint32_t    power_controller:1;     /**< error accessing power controller */
    uint32_t    fan_sensor:1;           /**< error accessing fan controller */
    
    uint32_t    thermal_sensor:1;       /**< error accessing thernal sensor */
    uint32_t    voltage_sensor:1;       /**< error accessing voltage sensor */
    uint32_t    /* reserved */ :2;

    uint32_t    serial_port:1;          /**< error accessing serial port */
    uint32_t    nvram:1;                /**< NVRAM error */
    uint32_t    rtc:1;                  /**< error accessing real-time clock */
    uint32_t    jtag:1;                 /**< error accessing scan controller */
    
    uint32_t    tod_battery:1;          /**< voltage loss from TOD battery 
                                             backup */
    uint32_t    /* reserved */ :1;
    uint32_t    heartbeat:1;            /**< heartbeat loss from surveillance 
                                             processor */
    uint32_t    surveillance:1;         /**< surveillance timeout */
    
    uint32_t    pcn_connection:1;       /**< power control network general
                                             connection failure */
    uint32_t    pcn_node:1;             /**< power control network node
                                             failure */
    uint32_t    /* reserved */ :2;
    uint32_t    pcn_access:1;           /**< error accessing power control
                                             network */
    uint32_t    /* reserved */ :3;

    uint32_t    sensor_token:32;
    uint32_t    sensor_index:32;

    uint32_t    /* reserved */:32;
    uint32_t    /* reserved */:32;
    uint32_t    /* reserved */:32;
};

/**
 * @struct rtas_ibm_diag_scn
 * @brief RTAS IBM diagnostic section
 */
struct rtas_ibm_diag_scn {
    struct scn_header shdr;
    uint32_t    event_id:32;
};

/**
 * @struct rtas_vend_errlog
 * @brief Vendor Specific Error Log section
 */
struct rtas_vend_errlog {
    struct scn_header shdr;
    char        vendor_id[4];
    uint32_t    vendor_data_sz;
    char        *vendor_data;
};
#endif
