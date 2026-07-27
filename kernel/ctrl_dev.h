/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __MTDPARTCTL_DEVICE_CHRDEV
#define __MTDPARTCTL_DEVICE_CHRDEV

#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/mtd/mtd.h>
#include <linux/mutex.h>
#include <linux/types.h>

#include "defs.h"

#define MTDPARTCTL_DEVICE_NAME "mtdpartctl"

struct mtd_context_partition {
	u64 offset;
	u64 length;
	bool writable;
	bool powerup_lock_enabled;

	char name[MTD_PARTITION_NAME_MAX_LENGTH];

	struct list_head node;
};

struct mtd_partitions_context {
	struct mutex lock;

	/* List of `struct mtd_context_partition`s */
	struct list_head partitions;
	size_t count;
};

struct mtdpartctl_device {
	/* Managed by the device class */
	struct class *device_class;
	struct mtd_info *mtd;
	dev_t devno;

	/* Managed by the character device */
	struct device *device;
	struct cdev ctrl_cdev;
	atomic_t already_open;
	struct mtd_partitions_context context;
};

int mtdpartctl_device_create(struct mtdpartctl_device *dev);
void mtdpartctl_device_destroy(struct mtdpartctl_device *dev);

#endif
