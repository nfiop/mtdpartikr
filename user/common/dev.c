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

int __go_mtdpartctl_add_new_partition(
    int fd, u64 offset, u64 length, char *name)
{
	size_t name_len = strlen(name);
	struct mtd_partition_info partition;

	partition.offset = offset;
	partition.length = length;

	memset(partition.name, 0, MTD_PARTITION_NAME_MAX_LENGTH);

	if (name_len <= MTD_PARTITION_NAME_MAX_LENGTH)
		memcpy(partition.name, name, name_len);
	else
		memcpy(partition.name, name, MTD_PARTITION_NAME_MAX_LENGTH);

	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_ADD_MTD_PARTITION, partition);
}

int mtdpartctl_recipe_add_part(int fd, struct ext_mtd_partition_info *partition)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_RECIPE_ADD_PART, partition);
}

int __go_mtdpartctl_recipe_add_part(int fd, u64 offset, u64 length, char *name,
    bool writable, bool powerup_lock_enabled)
{
	size_t name_len = strlen(name);
	struct ext_mtd_partition_info partition;

	partition.base.offset = offset;
	partition.base.length = length;

	memset(partition.base.name, 0, MTD_PARTITION_NAME_MAX_LENGTH);

	if (name_len <= MTD_PARTITION_NAME_MAX_LENGTH)
		memcpy(partition.base.name, name, name_len);
	else
		memcpy(
		    partition.base.name, name, MTD_PARTITION_NAME_MAX_LENGTH);

	partition.powerup_lock_enabled = powerup_lock_enabled;
	partition.readonly = !writable;
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_RECIPE_ADD_PART, &partition);
}

int mtdpartctl_recipe_del_part(int fd, size_t index)
{
	u32 tmp = index;
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_RECIPE_DEL_PART, &tmp);
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

int mtdpartctl_recipe_list_parts(int fd, struct recipe_partitions_list *list)
{
	INVOKE_IOCTL_WITH_RET_AS_ERRNO(
	    fd, MTDPARTCTL_IOC_LIST_RECIPE_PARTS, list);
}

int mtdpartctl_recipe_print_parts(int fd, size_t max_index)
{
	int ret;
	size_t idx;
	struct recipe_partitions_list *list;

	list = malloc(sizeof(struct recipe_partitions_list) +
		      (max_index * sizeof(struct ext_mtd_partition_info)));

	if (!list) {
		ret = -ENOMEM;
		goto exit;
	}

	list->read_count = max_index;

	ret = ioctl(fd, MTDPARTCTL_IOC_LIST_RECIPE_PARTS, list);
	if (ret < 0) {
		ret = -errno;
		goto exit;
	}

	for (idx = 0; idx < list->read_count; idx++) {
		printf("Partition %zu: %s, offset %llu, length %llu, readonly: "
		       "%s, powerup_lock_enabled: %s\n",
		    idx, list->parts[idx].base.name,
		    list->parts[idx].base.offset, list->parts[idx].base.length,
		    (const char *)(list->parts[idx].readonly ? "yes" : "no"),
		    (const char *)(list->parts[idx].powerup_lock_enabled
				       ? "yes"
				       : "no"));
	}

exit:
	free(list);
	return ret;
}