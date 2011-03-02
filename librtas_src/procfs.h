/**
 * @file procfs.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author John Rose <johnrose@us.ibm.com>
 */

#ifndef _PROCFS_H_
#define _PROCFS_H_

#define PFS_BUF_SIZE 4096

struct rtas_set_indicator {
	int indicator;		/* Indicator Token		*/
	int index;		/* Indicator Index		*/
	int new_value;		/* New value or State		*/
	int status;		/* Returned status		*/
};
struct rtas_get_sensor {
	int sensor;		/* Sensor Token			*/
	int index;		/* Sensor Index			*/
	int state;		/* Returned State of the sensor	*/
	int status;		/* Returned status		*/
};
struct rtas_cfg_connector {
	char workarea[PFS_BUF_SIZE];	/* Config work area  */
	int status;		  	/* Returned status		*/
};
struct rtas_get_power_level {
	int powerdomain;	/* Power Domain Token		*/
	int level;		/* Current Power Level 		*/
	int status;		/* Returned status		*/
};
struct rtas_set_power_level {
	int powerdomain;	/* Power Domain Token		*/
	int level;		/* Power Level Token		*/
	int setlevel;		/* Returned Set Power Level	*/
	int status;		/* Returned status		*/
};
struct rtas_get_sysparm {
	unsigned int parameter;		/* System parameter value	*/
	char data[PFS_BUF_SIZE];	/* Parameter data	*/
	unsigned int length;		/* Length of data buffer 	*/
	int status;			/* Returned status		*/
};

#endif
