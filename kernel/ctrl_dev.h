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
#include <linux/types.h>

#define MTDPARTCTL_DEVICE_NAME "mtdpartctl"

struct mtdparser_partition {
	u64 offset;
	u64 length;
	bool writable;
	bool powerup_lock_enabled;
};

struct mtdparser_context {
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
	struct mtdparser_context parser_context;
};

int mtdpartctl_device_create(struct mtdpartctl_device *dev);
void mtdpartctl_device_destroy(struct mtdpartctl_device *dev);

#endif
