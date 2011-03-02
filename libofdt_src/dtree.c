/**
 * @file dtree.c
 *
 * Copyright (C) 2008/2009 IBM Corporation
 * Common Public License Version 1.0 (see COPYRIGHT)
 *
 * @author Manish Ahuja mahuja@us.ibm.com
 */

#include "common.h"

struct node *treehead = NULL;
struct delete_struct *delete_list = NULL;

void __libofdt_fini() __attribute__((destructor));

/**
 * allocate_property
 * @brief allocates property
 *
 * @param name
 * @param path
 * @param value
 * @param size
 * @return property
 */
static struct property *allocate_property(const char *name, const char *path,
					  char *value)
{
	struct property *prop;
	int rc;

	prop = zalloc(sizeof(*prop));
	if (!prop)
		return NULL;

	if (!value) {
		FILE *fp;
		struct stat sb;

		rc = stat(path, &sb);
		if (rc)
			return NULL;

		prop->value = zalloc(sb.st_size + 1);
		if (prop->value) {
			free(prop);
			return NULL;
		}

		fp = fopen(path, "r");
		if (fp == NULL) {
			free(prop);
			return NULL;
		}

		rc = fread(prop->value, 1, sb.st_size, fp);
		fclose(fp);
	} else {
		int length = strlen(value);

		prop->value = zalloc(length + 1);
		if (!prop->value) {
			free(prop);
			return NULL;
		}

		strncpy(prop->value, value, length);
	}

	/* struct alredy nulled out, not copying last null */
	strncpy(prop->path, path, strlen(path));
	strncpy(prop->name, name, strlen(name));

	return prop;
}

/**
 * add_property
 * @brief adds property to head of queue and then calls allocate_property
 *
 * @param name
 * @param path
 * @param value
 * @param size
 */
void add_property(struct node *node, const char *name, char *value)
{
	struct property *prop;

	prop = allocate_property(name, node->path, value);

	prop->next = node->properties;
	node->properties = prop;
}

/**
 * allocate_node
 * @brief allocates the actual node.
 *
 * @param parent
 * @param path
 * @returns node
 */
static struct node *allocate_node(struct node *parent, const char* path)
{
	struct node *node;

	node = zalloc(sizeof(*node));
	if (node == NULL)
		return NULL;

	strncpy(node->path, path, strlen(path));
	node->parent = parent;

	return node;
}

/**
 * build_node
 * @brief the heart of this library, that builds and creates the whole tree.
 *
 * @param dirname
 * @param parent_node
 * @param parent_list
 * @returns node
 */
static struct node *build_node(const char *path,
			       struct dr_connector *my_drc_info)
{
	int rc;
	DIR *d;
	struct dirent *de;
	struct node *node;
	char buf[OFDT_MAX_PATH];
	struct dr_connector *child_drc_info;

	/* Check to see if we need to create drc info for any child nodes */
	if (file_exists(path, "ibm,drc-indexes"))
		child_drc_info = get_drc_info(path);
	else
		child_drc_info = my_drc_info;

	node = allocate_node(NULL, path);
	if (!node) {
		if (child_drc_info != my_drc_info)
			free_drc_info(child_drc_info);

		return NULL;
	}

	if (file_exists(path, "ibm,my-drc-index"))
		create_drc_properties(node, my_drc_info);

	d = opendir(path);
	if (!d) {
		if (child_drc_info != my_drc_info)
			free_drc_info(child_drc_info);

		return NULL;
	}

	while((de = readdir(d)) != NULL ) {
		struct stat sb;

		sprintf(buf, "%s/%s", path, de->d_name);
		rc = stat(buf, &sb);
		if (rc)
			break;

		if (S_ISDIR(sb.st_mode)) {
			struct node *child;

			child = build_node(buf, child_drc_info);

			child->sibling = node->child;
			node->child = child;
		} else if (S_ISREG(sb.st_mode)) {
			add_property(node, de->d_name, NULL);
		}
	}

	closedir(d);

	if (child_drc_info != my_drc_info)
			free_drc_info(child_drc_info);

	return node;
}

/**
 * add_node_to_dl
 * @brief Adds nodes to delete list.
 *
 * @param node to add
 */
static void add_node_to_dl(struct node *to_add)
{
	struct delete_struct *new;

	new = zalloc(sizeof(*new));
	if (!new)
		return;

	new->dnode = to_add;
	new->next = delete_list;
	delete_list = new;
}

/**
 * add_prop_to_dl
 * @brief Adds property to delete list.
 *
 * @param prop to add
 */
static void add_prop_to_dl(struct property *to_add)
{
	struct delete_struct *new;

	new = zalloc(sizeof(*new));
	if (!new)
		return;

	new->dprop = to_add;
	new->next = delete_list;
	delete_list = new;
}

/**
 * delete_properties
 * @brief deletes chained properties.
 *
 * @param prop to add
 */
static void delete_properties(struct node *to_delete)
{
	struct property *tmp, *prop;
	prop = to_delete->properties;

	while (prop != NULL) {
		tmp = prop;
		prop = prop->next;
		free(tmp);
	}
}

/**
 * remove_node
 * @brief removes sub-nodes and nodes recursively.
 *
 * @param node head.
 */
static void remove_node(struct node *head)
{
	struct node *tmp;

	while ((head->child) || (head->sibling)) {
		if (head->child) {
			remove_node(head->child);
			if (head->sibling == NULL && (head->properties)) {
				delete_properties(head);
				free(head);
				break;
			}
                }
		if (head->sibling) {
			if (head->properties)
				delete_properties(head);
                        tmp = head;
			head = head->sibling;
			free(tmp);
                }
        }
}

/**
 * delete_node_from_dl
 * @brief removes-any nodes being asked to delete.
 *
 * @param delete_node
 */
static void delete_node_from_dl(struct node *to_delete)
{
	struct delete_struct *tmp, *prev;

	tmp = delete_list;
	while (tmp) {
		if (tmp->dnode == to_delete) {
			remove_node(tmp->dnode);

			if (tmp == delete_list)
				delete_list = tmp->next;
			else
				prev->next = tmp->next;

			free(tmp);
			break;
		}

		prev = tmp;
		tmp = tmp->next;
	}
}

/**
 * delete_prop_from_dl
 * @brief removes-any properties being asked to delete.
 *
 * @param delete_prop
 */
static void delete_prop_from_dl(struct property *to_delete)
{
	struct delete_struct *tmp, *prev;

	tmp = delete_list;
	while (tmp) {
		if (tmp->dprop == to_delete) {
			free(tmp->dprop);

			if (tmp == delete_list)
				delete_list = tmp->next;
			else
				prev->next = tmp->next;

			free(tmp);
			return;
		}

		prev = tmp;
		tmp = tmp->next;
	}
}

/**
 * base_path
 * @brief builds appropriate path and returns it.
 *
 * The path specified can be either a full path to a device tree, or
 * a relative path to the real device tree at /proc/device-tree.  NOTE
 * that the relative path of NULL implies the base device tree at
 * /proc/device-tree
 *
 * @param dir
 */
static int path_to_full_dtpath(const char *user_path, char *full_path)
{
	struct stat sb;
	int rc;

	memset(full_path, 0, OFDT_MAX_PATH);

	if (!user_path) {
		/* NULL implies /proc/device-tree */
		sprintf(full_path, "%s", OFDT_BASE);
		return 0;
	}

	rc = stat(user_path, &sb);
	if (rc == 0) {
		/* Valid path specified, copy to full path */
		snprintf(full_path, OFDT_MAX_PATH, "%s", user_path);
		return 0;
	}

	/* build OFDT_BASE/user_path and validate */
	snprintf(full_path, OFDT_MAX_PATH, "%s/%s", OFDT_BASE, user_path);
	rc = stat(full_path, &sb);

	return rc;
}

/**
 * ofdt_get
 * @brief builds the device tree from the given path.
 *
 * @param path
 * @returns node
 */
struct node *ofdt_get(const char * path)
{
	char full_path[OFDT_MAX_PATH];
	struct dr_connector *drc_info;
	struct node *head;
	int rc;

	rc = path_to_full_dtpath(path, full_path);
	if (rc)
		return NULL;

	drc_info = find_drc_info(full_path);
	head = build_node(full_path, drc_info);
	if (head)
		add_node_to_dl(head);

	free_drc_info(drc_info);
	return head;
}

/**
 * ofdt_put
 * @brief releases the device tree from the given node pointer.
 *
 * @param path
 * @returns node
 */
void ofdt_put(struct node *delete)
{
	delete_node_from_dl(delete);
}

/**
 * ofdt_get_property
 * @brief returns the appropriate property pointer from given tree. It takes
 * path.
 *
 * @param node
 * @param path
 * @returns property
 */
struct property *ofdt_get_property(struct node *node, const char *name)
{
	struct property *prop;
	int len = strlen(name);

	for (prop = node->properties; prop; prop = prop->next) {
		if (strncmp(prop->name, name, len) == 0)
			return prop;
	}

	return NULL;
}

/**
 * ofdt_get_property_by_name
 * @brief builds and returns the appropriate property pointer from given path.
 *
 * @param path
 * @returns property
 */
struct property * ofdt_get_property_by_name(const char * dir)
{
	struct property *prop;
	char *name;
	char path[OFDT_MAX_PATH];
	int rc;

	rc = path_to_full_dtpath(dir, path);
	if (rc)
		return NULL;

	name = strrchr(path, '/');
	if (!name)
		return NULL;

	/* move past the '/' character */
	name++;

	prop = allocate_property(name, path, NULL);
	add_prop_to_dl(prop);
	return prop;
}

/**
 * ofdt_property_put
 * @brief Deletes properties from delete list.
 *
 * @param property
 */
void ofdt_property_put(struct property *to_delete)
{
	delete_prop_from_dl(to_delete);
}

#ifdef DEBUG
/**
 * traverse_devicetree
 * @brief Debug function, that prints nodes and properties.
 *
 * @param node_head
 */
void traverse_devicetree(struct node *head)
{
	struct property *prop;

	while ((head->child) || (head->sibling)) {
		if (head->child) {
			traverse_devicetree(head->child);
			if (head->sibling == NULL) {
				printf("NODE name is %s\n", head->path);
				for (prop = head->properties; prop != NULL;
							prop = prop->next) {
					printf("\tProp name is %s path is %s\n",
					       prop->name, prop->path);
				}
				break;
			}
		}
		if (head->sibling) {
			printf("NODE name is %s\n", head->path);
			for (prop = head->properties; prop != NULL;
							prop = prop->next) {
				printf("\tProp name is %s path is %s\n",
				       prop->name, prop->path);
			}
			head = head->sibling;
		}
	}
}
#endif

/**
 * __libofdt_fini()
 * @brief fini routine that cleans everything when library exits.
 *
 */
void __libofdt_fini()
{
	struct delete_struct *tmp, *prev;

	tmp = delete_list;
	while (tmp) {
		prev = tmp;
		if (tmp->dnode)
			remove_node(tmp->dnode);
		if (tmp->dprop)
			free(tmp->dprop);
		tmp = tmp->next;
		free(prev);
	}
}
