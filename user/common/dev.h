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
 * In contrast to partition added to a mtdpartctl context, this function should
 * be used for immediate action.
 *
 * @param fd        a file descriptor (associated with a master MTD device)
 * @param partition a pointer to `struct mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_add_new_partition(int fd, struct mtd_partition_info *partition);

/**
 * @brief Add partition to a mtdpartctl context
 *
 * In contrast to mtdpartctl_add_new_partition function, this function add new
 * partition to a mtdpartctl context, which is not an "immediate" action.
 *
 * @param fd        a file descriptor (which has the mtdpartctl context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_context_add_partition(
    int fd, struct ext_mtd_partition_info *partition);

/**
 * @brief Create MTD partitions using mtdpartctl context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_context_create_partitions(int fd);

/**
 * @brief Create MTD partitions using mtdpartctl context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_context_restart(int fd);

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

#endif
