/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef POWERPC_UAPI_LPARCTL_H
#define POWERPC_UAPI_LPARCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LPARCTL_IOCTL_BASE 0xb2

#define LPARCTL_IO(nr)         _IO(LPARCTL_IOCTL_BASE, nr)
#define LPARCTL_IOR(nr, type)  _IOR(LPARCTL_IOCTL_BASE, nr, type)
#define LPARCTL_IOW(nr, type)  _IOW(LPARCTL_IOCTL_BASE, nr, type)
#define LPARCTL_IOWR(nr, type) _IOWR(LPARCTL_IOCTL_BASE, nr, type)

/**
 * struct lparctl_get_system_parameter - System parameter retrieval.
 *
 * @rtas_status: (output) The result of the ibm,get-system-parameter RTAS
 *               call attempted by the kernel.
 * @token: (input) System parameter token as specified in PAPR+ 7.3.16
 *         "System Parameters Option".
 * @data: (input and output) If applicable, any required input data ("OS
 *        Service Entitlement Status" appears to be the only system
 *        parameter that requires this). If @rtas_status is zero on return
 *        from ioctl(), contains the value of the specified parameter. The
 *        first two bytes contain the (big-endian) length of valid data in
 *        both cases. If @rtas_status is not zero the contents are
 *        indeterminate.
 */
struct lparctl_get_system_parameter {
	__s32 rtas_status;
	__u16 token;
	union {
		__be16 length;
		__u8   data[4002];
	};
};

#define LPARCTL_GET_SYSPARM LPARCTL_IOWR(0x01, struct lparctl_get_system_parameter)

/**
 * struct lparctl_set_system_parameter - System parameter update.
 *
 * @rtas_status: (output) The result of the ibm,get-system-parameter RTAS
 *               call attempted by the kernel.
 * @token: (input) System parameter token as specified in PAPR+ 7.3.16
 *         "System Parameters Option".
 * @data: (input) The desired value for the parameter corresponding to
 *        @token. The first two bytes contain the (big-endian) length of
 *        valid data.
 */
struct lparctl_set_system_parameter {
	__s32 rtas_status;
	__u16 token;
	union {
		__be16 length;
		__u8   data[1026];
	};
};

#define LPARCTL_SET_SYSPARM LPARCTL_IOWR(0x02, struct lparctl_set_system_parameter)

#endif /* POWERPC_UAPI_LPARCTL_H */
