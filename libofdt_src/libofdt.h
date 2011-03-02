/**
 * @file libofdt.h
 *
 * Copyright (C) 2008/2009 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Manish Ahuja mahuja@us.ibm.com
 */

#ifndef _LIBOFDT_H_
#define _LIBOFDT_H_

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define OFDT_MAX_NAME 128 			/* Max name length */
#define OFDT_MAX_PATH 1024			/* Max Path length */

struct property {
	char name[OFDT_MAX_NAME];
	char path[OFDT_MAX_PATH];
	char *value;
	struct property *next;
	struct node * parent;
};

struct node {
	char name[OFDT_MAX_NAME];
	char path[OFDT_MAX_PATH];
	struct property *properties;
	struct node *child;
	struct node *parent;
	struct node *sibling;
};

extern struct node *ofdt_get(const char *);
extern void ofdt_put(struct node *);

extern struct property *ofdt_get_property(struct node *, const char *);
extern struct property *ofdt_get_property_by_name(const char *);
extern void ofdt_property_put(struct property *);

#endif /* _LIBOFDT_H_ */
