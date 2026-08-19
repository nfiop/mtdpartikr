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
 * @brief Add partition to a mtdpartctl recipe context
 *
 * In contrast to mtdpartctl_add_new_partition function, this function add new
 * partition to a mtdpartctl recipe context, which is not an "immediate" action.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_add_part(
    int fd, struct ext_mtd_partition_info *partition);

/**
 * @brief Add partition to a mtdpartctl recipe context (from Golang)
 *
 * In contrast to mtdpartctl_add_new_partition function, this function add new
 * partition to a mtdpartctl recipe context, which is not an "immediate" action.
 *
 * NOTE: This function is intended to be used in golang code, merely for it.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int __go_mtdpartctl_recipe_add_part(int fd, u64 offset, u64 length, char *name,
    bool writable, bool powerup_lock_enabled);

/**
 * @brief Delete partition to a mtdpartctl recipe context
 *
 * In contrast to mtdpartctl_add_new_partition function, this function add new
 * partition to a mtdpartctl recipe context, which is not an "immediate" action.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe context)
 * @param index     an index of the part to delete
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_del_part(int fd, size_t index);

/**
 * @brief List mtdpartctl recipe context parts
 *
 * This function will list a given set of partitions and their offsets and
 * and lengths for an MTD recipe, to the max count being specified in read_count
 * field of a given `struct recipe_partitions_list`.
 *
 * @param fd   file descriptor (associated with a master MTD device)
 * @param list a pointer to a list of partitions to be filled
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_list_parts(int fd, struct recipe_partitions_list *list);

/**
 * @brief Create MTD partitions using mtdpartctl recipe context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl recipe context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe context)
 * @param partition a pointer to `struct ext_mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_create_partitions(int fd);

/**
 * @brief Create MTD partitions using mtdpartctl recipe context
 *
 * By using this function, we instruct the kernel to register actual partitions
 * on an MTD master device using a mtdpartctl recipe context.
 *
 * @param fd        a file descriptor (which has the mtdpartctl recipe context)
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
 * @brief Add new partition using details in `struct mtd_partition_info`
 *
 * This function will try to create a new MTD partition on a MTD master device
 * using the given structure.
 * In contrast to partition added to a mtdpartctl recipe context, this function
 * should be used for immediate action.
 *
 * @param fd        a file descriptor (associated with a master MTD device)
 * @param partition a pointer to `struct mtd_partition_info` to be used
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_add_new_partition(int fd, struct mtd_partition_info *partition);

/**
 * @brief Add partition to a MTD device (from Golang)
 *
 * NOTE: This function is intended to be used in golang code, merely for it.
 *
 * @param fd        a file descriptor (for the mtdpartctl device which
 * represents the MTD)
 * @param offset    a partition offset
 * @param length    a partition length
 * @param name      a partition name
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int __go_mtdpartctl_add_new_partition(
    int fd, u64 offset, u64 length, char *name);

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

/**
 * @brief Print mtdpartctl recipe parts
 *
 * This function will print a given set of partitions and their offsets and
 * and lengths for an mtdpartctl recipe, to the max count being specified in
 * max_index parameter.
 *
 * @param fd   file descriptor
 * @param list a max index to enumerate
 * @return negative (errno) if ioctl call failed, otherwise 0.
 */
int mtdpartctl_recipe_print_parts(int fd, size_t max_index);

#endif
