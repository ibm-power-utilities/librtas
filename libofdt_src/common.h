/**
 * @file common.h
 *
 * Copyright (C) 2008/2009 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Manish Ahuja mahuja@us.ibm.com
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include "libofdt.h"

#define DRC_STR_MAX 48
#define OFDT_BASE "/proc/device-tree"

struct delete_struct {
	struct delete_struct *next;
	struct node *dnode;
	struct property *dprop;
};

struct dr_connector {
	char		name[DRC_STR_MAX];
	char		type[DRC_STR_MAX];
	unsigned int	index;
	unsigned int	powerdomain;
	struct dr_connector *next;
};

struct of_list_prop {
	char	*_data;
	char	*val;
	int	n_entries;
};

struct drc_prop_grp {
	struct of_list_prop drc_names;
	struct of_list_prop drc_types;
	struct of_list_prop drc_indexes;
	struct of_list_prop drc_domains;
};

struct dr_connector *get_drc_info(const char *);
void free_drc_info(struct dr_connector *);
char *of_to_full_path(const char *);
struct dr_connector *find_drc_info(const char *);
int file_exists(const char *, const char *);
void create_drc_properties(struct node *, struct dr_connector *);
void add_property(struct node *, const char *, char *);

static inline void *zalloc(int sz) {
	void *tmp = malloc(sz);
	if (tmp)
		memset(tmp, 0, sz);

	return tmp;
}

#endif /* COMMON_H_ */
