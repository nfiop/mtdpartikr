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

struct mtd_recipe_part {
	u64 offset;
	u64 length;
	bool writable;
	bool powerup_lock_enabled;

	char name[MTD_PARTITION_NAME_MAX_LENGTH];

	struct list_head node;
};

struct mtd_recipe {
	struct mutex lock;

	/* List of `struct mtd_recipe_part`s */
	struct list_head partitions;
	size_t parts_count;
};

struct proxy_mtd {
	struct mtd_info *mtd;

	/* Lock to protect access to about_to_respawn and refcount */
	struct mutex lock;

	int use_refcnt;
	bool about_to_respawn;
};

struct mtdpartctl_device {
	/* Managed by the device class */
	struct class *device_class;
	struct mtd_info *backing_mtd;
	dev_t devno;

	/* Managed by the character device */
	struct device *device;
	struct cdev ctrl_cdev;
	atomic_t already_open;
	struct mtd_recipe recipe;

	/* A proxy MTD which is backed by backing_mtd, with a mutex and a
	 * ref-count for ensuring that we don't try to "respawn" an MTD with
	 * new partitions (or just respawn master MTD with no partitions)
	 * when someone uses it somehow.
	 */
	struct proxy_mtd proxy;
};

int mtdpartctl_device_create(struct mtdpartctl_device *dev);
void mtdpartctl_device_destroy(struct mtdpartctl_device *dev);

#endif
