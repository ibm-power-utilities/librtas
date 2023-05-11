/**
 * @file syscall_rmo.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <inttypes.h>
#include "internal.h"
#include "librtas.h"

#define MAX_PAGES 64
#define MAX_PATH_LEN 80

struct workarea_config {
	int init_done;
	int lockfile_fd;
	struct region kern_region;
	uint64_t pages_map;
};

static const char *rmo_filename = "rmo_buffer";
static const char *devmem_path = "/dev/mem";
static const char *lockfile_path = "/var/lock/LCK..librtas";

static struct workarea_config wa_config = {
	.lockfile_fd = -1,
	.init_done = 0,
	.pages_map = 0ll,
};

/**
 * open_proc_rtas_file
 * @brief Open the proc rtas file
 *
 * @param name filename to open
 * @param mode mode to open file in
 * @return results of open() call
 */
static int open_proc_rtas_file(const char *name, int mode)
{
	const char *proc_rtas_paths[] = { "/proc/ppc64/rtas", "/proc/rtas" };
	char full_name[MAX_PATH_LEN];
	int npaths;
	int fd;
	int i;

	npaths = sizeof(proc_rtas_paths) / sizeof(char *);
	for (i = 0; i < npaths; i++) {
		sprintf(full_name, "%s/%s", proc_rtas_paths[i], name);
		fd = open(full_name, mode, S_IRUSR | S_IWUSR);
		if (fd >= 0)
			break;
	}

	if (fd < 0)
		dbg("Failed to open %s\n", full_name);

	return fd;
}

/**
 * read_kregion_bounds
 * @brief Read the kernel region bounds for RMO memory
 *
 * @param kregion
 * @return 0 on success, !0 otherwise
 */
static int read_kregion_bounds(struct region *kregion)
{
	char *buf;
	int fd;
	int rc;

	fd = open_proc_rtas_file(rmo_filename, O_RDONLY);
	if (fd < 0) {
		dbg("Could not open workarea file\n");
		return RTAS_IO_ASSERT;
	}

	rc = read_entire_file(fd, &buf, NULL);
	close(fd);
	if (rc) {
		free(buf);
		return rc;
	}

	sscanf(buf, "%" SCNx64 " %x", &kregion->addr, &kregion->size);
	free(buf);

	if (!(kregion->size && kregion->addr) ||
	    (kregion->size > (WORK_AREA_SIZE * MAX_PAGES))) {
		dbg("Unexpected kregion bounds\n");
		return RTAS_IO_ASSERT;
	}

	return 0;
}

/**
 * get_bits
 *
 * @param lobit
 * @param hibit
 * @param mask
 * @return 0 on success, !0 otherwise
 */
static uint64_t get_bits(short lobit, short hibit, uint64_t mask)
{
	short num_bits = hibit - lobit + 1;
	uint64_t ones_mask = (1ll << (num_bits)) - 1;

	return ((mask >> lobit) & ones_mask);
}

/**
 * set_bits
 *
 * @param lobit
 * @param hibit
 * @param value
 * @param mask
 * @return 0 on success, !0 otherwise
 */
static void set_bits(short lobit, short hibit, uint64_t value,
			    uint64_t *mask)
{
	short num_bits = hibit - lobit + 1;
	uint64_t ones_mask = (1ll << (num_bits)) - 1;

	*mask &= ~(ones_mask << lobit);
	*mask |= value << lobit;
}

/**
 * acquire_file_lock
 *
 * @param start
 * @param size
 * @return 0 on success, !0 otherwise
 */
static int acquire_file_lock(off_t start, size_t size)
{
	struct flock flock;
	int rc;

	/* Lazily open lock file */
	if (wa_config.lockfile_fd < 0) {
		wa_config.lockfile_fd = open(lockfile_path, O_CREAT | O_RDWR,
					     S_IRUSR | S_IWUSR);
		if (wa_config.lockfile_fd < 0) {
			dbg("could not open lockfile %s\n", lockfile_path);
			return RTAS_IO_ASSERT;
		}
	}

	flock.l_start = start;
	flock.l_type = F_WRLCK;
	flock.l_whence = SEEK_SET;
	flock.l_len = size;
	flock.l_pid = getpid();

	rc = fcntl(wa_config.lockfile_fd, F_SETLKW, &flock);
	if (rc < 0) {
		/* Expected to fail for regions used by other processes */
		dbg("fcntl failed for [0x%lx, 0x%zx]\n", start, size);
		return RTAS_IO_ASSERT;
	}

	return 0;
}

/**
 * release_file_lock
 *
 * @param start
 * @param size
 * @return 0 on success, !0 otherwise
 */
static int release_file_lock(off_t start, size_t size)
{
	struct flock flock;
	int rc;

	flock.l_start = start;
	flock.l_type = F_UNLCK;
	flock.l_whence = SEEK_SET;
	flock.l_len = size;
	flock.l_pid = getpid();

	rc = fcntl(wa_config.lockfile_fd, F_SETLK, &flock);
	if (rc < 0) {
		dbg("fcntl failed for [0x%lx, 0x%zx]\n", start, size);
		return RTAS_IO_ASSERT;
	}

	return 0;
}

/**
 * get_phys_region
 *
 * @param size
 * @param phys_addr
 * @return 0 on success, !0 otherwise
 */
static int get_phys_region(size_t size, uint32_t * phys_addr)
{
	struct region *kregion = &wa_config.kern_region;
	uint32_t addr = 0;
	uint64_t bits;
	int n_pages;
	int i;

	if (size > kregion->size) {
		dbg("Invalid buffer size 0x%zx requested\n", size);
		return RTAS_IO_ASSERT;
	}

	n_pages = size / WORK_AREA_SIZE;

	for (i = 0; i < MAX_PAGES; i++) {
		if ((i * WORK_AREA_SIZE) >= kregion->size)
			break;

		bits = get_bits(i, i + n_pages - 1, wa_config.pages_map);
		if (bits == 0ll) {
			if (acquire_file_lock(i, n_pages) == 0) {
				set_bits(i, i + n_pages - 1,
					 (1 << n_pages) - 1,
					 &wa_config.pages_map);
				addr = kregion->addr + (i * WORK_AREA_SIZE);
				break;
			}
		}
	}

	if (!addr) {
		dbg("Could not find available workarea space\n");
		return RTAS_IO_ASSERT;
	}

	*phys_addr = addr;
	return 0;
}

/**
 * release_phys_region
 *
 * @param phys_addr
 * @param size
 * @return 0 on success, !0 otherwise
 */
static int release_phys_region(uint32_t phys_addr, size_t size)
{
	struct region *kregion = &wa_config.kern_region;
	int first_page;
	int n_pages;
	uint64_t bits;
	int rc;

	if (size > kregion->size) {
		dbg("Invalid buffer size 0x%zx requested\n", size);
		return RTAS_IO_ASSERT;
	}

	first_page = (phys_addr - kregion->addr) / WORK_AREA_SIZE;
	n_pages = size / WORK_AREA_SIZE;

	bits = get_bits(first_page, first_page + n_pages - 1,
			wa_config.pages_map);
	if (bits != ((1 << n_pages) - 1)) {
		dbg("Invalid region [0x%x, 0x%zx]\n", phys_addr, size);
		return RTAS_IO_ASSERT;
	}

	set_bits(first_page, first_page + n_pages - 1, 0, &wa_config.pages_map);

	rc = release_file_lock(first_page, n_pages);

	return rc;
}

/**
 * init_workarea_config
 *
 * @return 0 on success, !0 otherwise
 */
static int init_workarea_config(void)
{
	int rc;

	/* Read bounds of reserved kernel region */
	rc = read_kregion_bounds(&wa_config.kern_region);
	if (rc)
		return rc;

	wa_config.init_done = 1;

	return 0;
}

/**
 * mmap_dev_mem
 *
 * @param phys_addr
 * @param size
 * @param buf
 * @return 0 on success, !0 otherwise
 */
static int mmap_dev_mem(uint32_t phys_addr, size_t size, void **buf)
{
	void *newbuf;
	int fd;

	fd = open(devmem_path, O_RDWR);
	if (fd < 0) {
		dbg("Failed to open %s\n", devmem_path);
		return RTAS_IO_ASSERT;
	}

	newbuf = mmap((void *)0, size, PROT_READ | PROT_WRITE,
		      MAP_SHARED, fd, phys_addr);
	close(fd);

	if (newbuf == MAP_FAILED) {
		dbg("mmap failed\n");
		return RTAS_IO_ASSERT;
	}

	*buf = newbuf;
	return 0;
}

/**
 * munmap_dev_mem
 *
 * @param buf
 * @param size
 * @return 0 on success, !0 otherwise
 */
static int munmap_dev_mem(void *buf, size_t size)
{
	int rc;
	int fd;

	fd = open(devmem_path, O_RDWR);
	if (fd < 0) {
		dbg("Failed to open %s\n", devmem_path);
		return RTAS_IO_ASSERT;
	}

	rc = munmap(buf, size);
	close(fd);
	if (rc < 0) {
		dbg("munmap failed\n");
		return RTAS_IO_ASSERT;
	}

	return 0;
}

/**
 * interface_exists
 *
 * @return 0 on success, !0 otherwise
 */
int interface_exists(void)
{
	int fd = open_proc_rtas_file(rmo_filename, O_RDONLY);
	int exists;

	exists = (fd >= 0);

	if (exists)
		close(fd);

	return exists;
}

/**
 * rtas_free_rmo_buffer
 * @brief free the rmo buffer used by librtas
 *
 * @param buf virtual address of mmap()'ed buffer
 * @param phys_addr physical address of low mem buffer
 * @param size size of buffer
 * @return 0 on success, !0 otherwise
 *	RTAS_FREE_ERR - Free called before get
 * 	RTAS_IO_ASSERT - Unexpected I/O Error
 */
int rtas_free_rmo_buffer(void *buf, uint32_t phys_addr, size_t size)
{
	int n_pages;
	int rc;

	rc = sanity_check();
	if (rc)
		return rc;

	n_pages = size / WORK_AREA_SIZE;

	/* Check for multiple of page size */
	if (size % WORK_AREA_SIZE) {
		/* Round up to multiple of WORK_AREA_SIZE */
		n_pages++;
		size = n_pages * WORK_AREA_SIZE;
	}

	if (!wa_config.init_done) {
		dbg("Attempting to free before calling get()\n");
		return RTAS_FREE_ERR;
	}

	rc = munmap_dev_mem(buf, size);
	if (rc) {
		(void) release_phys_region(phys_addr, size);
		return rc;
	}

	rc = release_phys_region(phys_addr, size);

	return rc;
}

/**
 * rtas_get_rmo_buffer
 * @brief Retrive the RMO buffer used by librtas
 *
 * On successful completion the buf parameter will reference an allocated
 * area of RMO memory and the phys_addr parameter will refernce the
 * physical address of the RMO buffer.
 *
 * @param size Size of requested region.  Must be a multiple of 4096.
 * @param buf Assigned to mmap'ed buffer of acquired region
 * @param phys_addr  Assigned to physical address of acquired region
 * @return 0 on success, !0 otherwise
 * 	RTAS_NO_MEM - Out of heap memory
 * 	RTAS_NO_LOWMEM - Out of rmo memory
 * 	RTAS_IO_ASSERT - Unexpected I/O Error
 */
int rtas_get_rmo_buffer(size_t size, void **buf, uint32_t * phys_addr)
{
	uint32_t addr;
	int n_pages;
	int rc;

	rc = sanity_check();
	if (rc)
		return rc;

	dbg("RMO buffer request, size: %zd\n", size);

	n_pages = size / WORK_AREA_SIZE;

	if (size % WORK_AREA_SIZE) {
		/* Round up to multiple of WORK_AREA_SIZE */
		n_pages++;
		size = n_pages * WORK_AREA_SIZE;
	}

	if (!wa_config.init_done) {
		rc = init_workarea_config();
		if (rc)
			return rc;
	}

	rc = get_phys_region(size, &addr);
	if (rc)
		return rc;

	rc = mmap_dev_mem(addr, size, buf);
	if (rc) {
		(void) release_phys_region(addr, size);
		return rc;
	}

	*phys_addr = addr;
	return 0;
}
