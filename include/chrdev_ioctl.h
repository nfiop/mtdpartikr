/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

// chrdev_ioctl.h - shared UAPI header for kernel and userspace
#ifndef MTDPARTCTL_IOCTL_H
#define MTDPARTCTL_IOCTL_H

#include "defs.h"

// Detect whether we are compiling in the kernel or userspace
#ifdef __KERNEL__
#include <linux/ioctl.h> /* _IO, _IOR, _IOW */
#else
#include <sys/ioctl.h>
#endif

/* Magic number for ioctl commands */
#define MTDPARTCTL_IOC_MAGIC 'T'

struct mtdpartctl_info {
	__u32 backend_mtd_index;
	__u32 erase_sector_size; /* Size of erase sector */
	__u32 reserved[6];	 /* reserved for future expansion */
};

struct mtd_partition_info {
	__u64 offset;
	__u64 length;
	char name[MTD_PARTITION_NAME_MAX_LENGTH];
};

struct ext_mtd_partition_info {
	struct mtd_partition_info base;

	/* Extended flags, that are added to add_flags and mask_flags
	 * in a `struct mtd_partition` field.
	 * They're normally not ON (set to false), so adding them is like this:
	 *  - For readonly, we should mask MTD_WRITABLE in mask_flags
	 *  - For powerup_lock, we should add MTD_POWERUP_LOCK in add_flags
	 */
	bool readonly;
	bool powerup_lock_enabled;
};

/* ioctl commands */
#define MTDPARTCTL_IOC_GET_INFO _IOR(MTDPARTCTL_IOC_MAGIC, 0, struct mtdpartctl_info)
#define MTDPARTCTL_IOC_ADD_NEW_PART                                            \
	_IOW(MTDPARTCTL_IOC_MAGIC, 1, struct mtd_partition_info)
#define MTDPARTCTL_IOC_DEL_PART _IOW(MTDPARTCTL_IOC_MAGIC, 2, u32)
#define MTDPARTCTL_IOC_ADD_CONTEXT_PART                                        \
	_IOW(MTDPARTCTL_IOC_MAGIC, 3, struct ext_mtd_partition_info)
#define MTDPARTCTL_IOC_CREATE_PARTITIONS _IO(MTDPARTCTL_IOC_MAGIC, 4)
#define MTDPARTCTL_IOC_RESTART_PARTITIONS_CONTEXT _IO(MTDPARTCTL_IOC_MAGIC, 5)

#endif
