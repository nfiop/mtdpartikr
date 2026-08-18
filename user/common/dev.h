/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __USER__DEV_H_
#define __USER__DEV_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "chrdev_ioctl.h"
#include "defs.h"

/**
 * @brief Open a proxy device file descriptor based on a MTD index string
 *
 * This function is mainly used by utests - it's lean, and doesn't do much
 * and also quite noisy about what it does which is perfect for verbose apps.
 *
 * @param index a string containing an mtdpartctl index (e.g. "0" for
 *              /dev/mtdpartctl0)
 * @return non-negative file descriptor number if successful, other negative
 *         number for error
 */
int open_mtdpartctl_device_by_argv_index(const char *index);

/**
 * @brief Get info on mtdpartctl device
 *
 * This function returns details in a given `struct mtdpartctl_info`.
 *
 * @param info a pointer to `struct mtdpartctl_info` to be filled
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_get_info(int fd, struct mtdpartctl_info *info);

/**
 * @brief Add new partition using details in `struct mtd_partition_info`
 *
 * This function will try to create a new MTD partition on a MTD master device
 * using the given structure.
 * In contrast to partition added to a mtdpartctl recipe_context, this function
 * should be used for immediate action.
 *
 * @param fd        a file descriptor (associated with a master MTD device)
 * @param partition a pointer to `struct mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_add_new_partition(int fd, struct mtd_partition_info *partition);

/**
 * @brief Add partition to a mtdpartctl recipe_context
 *
 * In contrast to mtdpartctl_add_new_partition function, this function add new
 * partition to a mtdpartctl recipe_context, which is not an "immediate" action.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe_context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_add_part(
    int fd, struct ext_mtd_partition_info *partition);

int mtdpartctl_recipe_del_part(int fd, size_t index);

int mtdpartctl_recipe_list_partition(
    int fd, struct ext_mtd_partition_info *partitions, size_t max_index);

/**
 * @brief Create MTD partitions using mtdpartctl recipe_context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl recipe_context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe_context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_create_partitions(int fd);

/**
 * @brief Create MTD partitions using mtdpartctl recipe_context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl recipe_context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe_context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_restart(int fd);

/**
 * @brief Delete all MTD partitions of a master MTD device
 *
 * This function should clean all MTD partitions of a given master MTD device.
 *
 * @param fd        a file descriptor (associated with a master MTD device)
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_delete_mtd_partitions(int fd);

/**
 * @brief Remove a partition using an index
 *
 * This function will try to remove a MTD partition on a MTD master device
 * given an index, which is relative to the master MTD (i.e. 0 for first
 * partition 1 for second partitions, etc).
 *
 * @param fd   file descriptor (associated with a master MTD device)
 * @param idx  relative index of the MTD partition to remove
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_delete_partition(int fd, u32 idx);

/**
 * @brief List MTD partitions
 *
 * This function will list a given set of partitions and their offsets and
 * and lengths for an MTD, to the max count being specified in read_count field
 * of a given `struct mtd_partitions_list`.
 *
 * @param fd   file descriptor (associated with a master MTD device)
 * @param list a pointer to a list of partitions to be filled
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_list_partitions(int fd, struct mtd_partitions_list *list);

#endif
