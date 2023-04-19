/**
 * @file ofdt.c
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
 * @author John Rose <johnrose@us.ibm.com>
 */

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <byteswap.h>
#include "librtas.h"
#include "internal.h"

static const char *ofdt_rtas_path = "/proc/device-tree/rtas";

/**
 * open_prop_file
 * @brief Open the property name located at the specified property path
 *
 * @param prop_path path to the property name
 * @param prop_name property name
 * @param fd reference to file descriptor
 * @return 0 on success, -1 on failure
 */
static int open_prop_file(const char *prop_path, const char *prop_name, int *fd)
{
	char *path;
	int len;

	/* allocate enough for two string, a slash and trailing NULL */
	len = strlen(prop_path) + strlen(prop_name) + 1 + 1;
	path = malloc(len);
	if (path == NULL) {
		errno = ENOMEM;
		return -1;
	}

	snprintf(path, len, "%s/%s", prop_path, prop_name);

	*fd = open(path, O_RDONLY);
	free(path);
	if (*fd < 0) {
		errno = ENOSYS;
		return -1;
	}

	return 0;
}

/**
 * get_property
 * @brief Open a property file and read in its contents
 *
 * @param prop_path path to the property file
 * @param prop_name propery name
 * @param prop_val
 * @param prop_len
 * @return 0 on success, !0 otherwise
 */
static int get_property(const char *prop_path, const char *prop_name,
			char **prop_val, size_t *prop_len)
{
	int rc, fd;

	rc = open_prop_file(prop_path, prop_name, &fd);
	if (rc)
		return rc;

	rc = read_entire_file(fd, prop_val, prop_len);
	close(fd);

	return rc;
}

/**
 * rtas_token
 * @brief Retrive a rtas token for a given name
 *
 * @param call_name rtas name to retrieve token for
 * @return 0 on success, !0 otherwise
 */
int rtas_token(const char *call_name)
{
	char *prop_buf = NULL;
	size_t len;
	int rc;

	rc = get_property(ofdt_rtas_path, call_name, &prop_buf, &len);
	if (rc < 0) {
		if (prop_buf)
			free(prop_buf);

		return RTAS_UNKNOWN_OP;
	}

#if __BYTE_ORDER == __LITTLE_ENDIAN
	rc = bswap_32(*(int *)prop_buf);
#else
	rc = *(int *)prop_buf;
#endif
	free(prop_buf);

	return rc;
}

#define BLOCK_SIZE 4096

/**
 * read_entire_file
 * @brief Read in an entire file into the supplied buffer
 *
 * @param fd opened file descriptor for file to read
 * @param buf buffer to read file into
 * @param len variable to return amount read into buffer
 * @return 0 on success, !0 otherwise
 */
int read_entire_file(int fd, char **buf, size_t * len)
{
	size_t buf_size = 0;
	size_t off = 0;
	int rc;

	*buf = NULL;
	do {
		buf_size += BLOCK_SIZE;
		if (*buf == NULL)
			*buf = malloc(buf_size);
		else
			*buf = realloc(*buf, buf_size);

		if (*buf == NULL) {
			errno = ENOMEM;
			return -1;
		}

		rc = read(fd, *buf + off, BLOCK_SIZE);
		if (rc < 0) {
			dbg("read failed\n");
			errno = EIO;
			return -1;
		}

		off += rc;
	} while (rc == BLOCK_SIZE);

	if (len)
		*len = off;

	return 0;
}
