/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dev.h"
#include "ioctl_utils.h"

int open_mtdpartctl_device_by_argv_index(const char *index)
{
	unsigned long idx;
	char *end;
	int fd;
	char path[64];

	errno = 0;
	idx = strtoul(index, &end, 10);

	/* Validation */
	if (errno != 0) {
		perror("strtoul");
		return 1;
	}

	if (*end != '\0') {
		fprintf(
		    stderr, "Invalid input: trailing characters '%s'\n", end);
		return 1;
	}

	if (idx > 1000) {
		fprintf(stderr, "Index too large (max 1000)\n");
		return 1;
	}

	snprintf(path, sizeof(path), "/dev/mtdpartctl%lu", idx);

	fprintf(stderr, "Opening %s\n", path);

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	fprintf(stderr, "Device opened successfully\n");
	return fd;
}

int mtdpartctl_get_info(int fd, struct mtdpartctl_info *info)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(fd, MTDPARTCTL_IOC_GET_INFO, info);
}

int mtdpartctl_add_new_partition(int fd, struct mtd_partition_info *partition)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_ADD_MTD_PARTITION, partition);
}

int mtdpartctl_recipe_add_part(int fd, struct ext_mtd_partition_info *partition)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_RECIPE_ADD_PART, partition);
}

int mtdpartctl_recipe_create_partitions(int fd)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_RECIPE_CREATE_PARTITIONS, 0);
}

int mtdpartctl_recipe_restart(int fd)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(fd, MTDPARTCTL_IOC_RESTART_RECIPE, 0);
}

int mtdpartctl_delete_mtd_partitions(int fd)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_DELETE_MTD_PARTITIONS, 0);
}

int mtdpartctl_delete_partition(int fd, u32 idx)
{
	u32 tmp = idx;
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_DEL_MTD_PARTITION, &tmp);
}

int mtdpartctl_list_partitions(int fd, struct mtd_partitions_list *list)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_GET_PARTITION_LIST, list);
}
