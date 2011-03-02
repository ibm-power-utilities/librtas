/**
 * @file drc.c
 * @brief Common routines for building DRC Info.
 * @author Borrowed/Adapted code from drmgr authored by "Nathan Fontenont".
 * @author Manish Ahuja mahuja@us.ibm.com
 *
 * Copyright (C) IBM Corporation 2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdint.h>
#include "common.h"

/**
 * get_property
 * @brief retrieve a device-tree property from /proc
 *
 * @param path path to the property to retrieve
 * @param name name of the property to retrieve
 * @param prop of_list_prop to retrieve property for
 * @returns 0 on success, !0 otherwise
 */
static int get_property(const char *path, const char *property,
			void *buf, int buf_sz)
{
	FILE *fp;
	int rc;
	char dir[OFDT_MAX_PATH];

	if ((path == NULL) || (property == NULL))
		return -1;

	sprintf(dir, "%s/%s", path, property);
	fp = fopen(dir, "r");
	if (fp == NULL)
		return -1;

	rc = fread(buf, buf_sz, 1, fp);

	fclose(fp);
	return 0;
}

/**
 * get_proprety_size
 * @brief Retrieve the size of a device tree property
 *
 * @param path full path to property
 * @param name property name
 * @returns size of property
 */
static int get_property_size(const char *path, const char *name)
{
	char file_path[OFDT_MAX_PATH];
	struct stat sb;
	int rc;

	sprintf(file_path, "%s/%s", path, name);
	rc = stat(file_path, &sb);

	return sb.st_size;
}

/**
 * get_of_list_prop
 * @breif retrieve the specified open firmware property list
 *
 * @param full_path
 * @param prop_name
 * @param prop
 * @returns 0 on success, !0 otherwise
 */
static int get_of_list_prop(const char *full_path, char *prop_name,
			    struct of_list_prop *prop)
{
	int rc, size;

	size = get_property_size(full_path, prop_name);
	prop->_data = zalloc(size);
	if (prop->_data == NULL)
		return -1;

	rc = get_property(full_path, prop_name, prop->_data, size);
	if (rc) {
		free(prop->_data);
		return -1;
	}

	prop->n_entries = *(uint *)prop->_data;
	prop->val = prop->_data + sizeof(uint);

	return 0;
}

/**
 * get_drc_prop_grp
 *
 * @param full_path
 * @param group
 * @returns 0 on success, !0 otherwise
 */
static int get_drc_prop_grp(const char *full_path, struct drc_prop_grp *group)
{
	int rc;

	memset(group, 0, sizeof(*group));

	rc = get_of_list_prop(full_path, "ibm,drc-names", &group->drc_names);
	if (rc)
		return rc;

	rc = get_of_list_prop(full_path, "ibm,drc-types", &group->drc_types);
	if (rc)
		return rc;

	rc = get_of_list_prop(full_path, "ibm,drc-indexes",
			      &group->drc_indexes);
	if (rc)
		return rc;

	rc = get_of_list_prop(full_path, "ibm,drc-power-domains",
			      &group->drc_domains);
	if (rc)
		return rc;

	return 0;
}

/**
 * free_drc_props
 * @brief free the properties associated with a drc group
 *
 * @param group
 */
static void free_drc_props(struct drc_prop_grp *group)
{
	if (group->drc_names.val)
		free(group->drc_names._data);
	if (group->drc_types.val)
		free(group->drc_types._data);
	if (group->drc_indexes.val)
		free(group->drc_indexes._data);
	if (group->drc_domains.val)
		free(group->drc_domains._data);
}

/**
 * build_connectors_group
 *
 * @param group
 * @param n_entries
 * @param list
 * @returns 0 on success, !0 otherwise
 */
static int build_connectors_list(struct drc_prop_grp *group, int n_entries,
				 struct dr_connector *list)
{
	struct dr_connector *entry;
	unsigned int *index_ptr;
	unsigned int *domain_ptr;
	char *name_ptr;
	char *type_ptr;
	int i;

	index_ptr = (unsigned int *)group->drc_indexes.val;
	domain_ptr = (unsigned int *)group->drc_domains.val;
	name_ptr = group->drc_names.val;
	type_ptr = group->drc_types.val;

	for (i = 0; i < n_entries; i++) {
		entry = &list[i];

		entry->index = *(index_ptr++);
		entry->powerdomain = *(domain_ptr++);

		strncpy(entry->name, name_ptr, DRC_STR_MAX);
		name_ptr += strlen(name_ptr) + 1;

		strncpy(entry->type, type_ptr, DRC_STR_MAX);
		type_ptr += strlen(type_ptr) + 1;

		if (i == (n_entries - 1))
			entry->next = NULL;
		else
			entry->next = &list[i+1];
	}

	return 0;
}

/**
 * get_dr_connectors
 *
 * NOTE:Callers of this function are expected to free drc_list themselves
 *
 * @param of_path
 * @param drc_list
 * @param n_drcs
 * @returns 0 on success, !0 otherwise
 */
struct dr_connector *get_drc_info(const char *of_path)
{
	struct dr_connector *list = NULL;
	struct of_list_prop *drc_names;
	struct drc_prop_grp prop_grp;
	int n_drcs;
	int rc;

	rc = get_drc_prop_grp(of_path, &prop_grp);
	if (rc)
		return NULL;

	drc_names = &prop_grp.drc_names;
	n_drcs = drc_names->n_entries;

	list = zalloc(n_drcs * sizeof(struct dr_connector));
	if (list)
		rc = build_connectors_list(&prop_grp, n_drcs, list);

	free_drc_props(&prop_grp);
	return list;
}

/**
 * free_drc_info
 *
 * @param drc_list
 */
void free_drc_info(struct dr_connector *drc_list)
{
	free(drc_list);
}

/**
 * file_exists
 *
 * @brief Function that checks whether ibm,my-drc-index exists.
 *
 * @param path
 * @returns 0 on success, !0 otherwise
 */
int file_exists(const char *path, const char *fname)
{
	int rc;
	struct stat sb;
	char file_path[OFDT_MAX_PATH];

	sprintf(file_path, "%s/%s", path, fname);
	rc = stat(file_path, &sb);

	return !rc;
}

/**
 * create_drc_props
 *
 * @brief Function that creates drc,type/name/powerdomain.
 *
 * @param dr_connector
 * @param parent node
 * @param path
 */
void create_drc_properties(struct node *node, struct dr_connector *drc_list)
{
	char powerdomain[OFDT_MAX_NAME];
	struct dr_connector *drc;
	int rc;
	uint32_t drc_index;

	if (drc_list == NULL)
		return;

	rc = get_property(node->path, "ibm,my-drc-index", &drc_index,
			  sizeof(drc_index));
	if (rc)
		return;

	for (drc = drc_list; drc; drc = drc->next) {
		if (drc_index == drc->index)
			break;
	}

	if (!drc)
		return

	add_property(node, "ibm,drc-name", drc->name);
	add_property(node, "ibm,drc-type", drc->type);

	sprintf(powerdomain, "%u", drc->powerdomain);
	add_property(node, "ibm,drc-powerdomain", powerdomain);
}

/**
 * find_drc_info
 *
 * @brief Function that parses prev sub-directories until it finds
 * ibm,drc-index file
 *
 * @param path
 * @returns dr_connector type.
 */
struct dr_connector *find_drc_info(const char *path)
{
	struct stat sb;
	int rc;
	char tmp_path[OFDT_MAX_PATH];
	char buf[OFDT_MAX_PATH];
	char *slash;

	sprintf(buf, "%s", path);

	do {
		sprintf(tmp_path, "%s/%s", buf, "ibm,drc-indexes");
		rc = stat(buf, &sb);
		if (rc == 0)
			return get_drc_info(buf);

		slash = strrchr(buf, '/');
		if (slash)
			*slash = '\0';
	} while (slash);

	return NULL;
}
