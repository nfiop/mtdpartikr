/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "ctrl_dev.h"
#include "ctrl_dev_class.h"

#include "chrdev_ioctl.h"

static int mtdpartctl_chrdev_open(struct inode *inode, struct file *filp)
{
	dev_t dev = inode->i_rdev;
	int minor = MINOR(dev);

	struct mtdpartctl_device *mtdpartctl_dev =
	    mtdpartctl_device_resolve_by_minor(minor);
	if (!mtdpartctl_dev)
		return -ENODEV;

	// Don't allow opening more than once, as we can't really
	// handle multiple clients anyway.
	if (atomic_cmpxchg(&mtdpartctl_dev->already_open, 0, 1)) {
		return -EBUSY;
	}

	filp->private_data = mtdpartctl_dev;
	return 0;
}

static int mtdpartctl_chrdev_release(struct inode *inode, struct file *filp)
{
	struct mtdpartctl_device *dev = filp->private_data;
	atomic_set(&dev->already_open, 0);
	return 0;
}

static long mtdpartctl_chrdev_ioctl(
    struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct mtdpartctl_device *dev = filp->private_data;
	struct mtd_info *backend;

	WARN_ON(dev == NULL);
	if (dev == NULL)
		return -EIO;

	backend = dev->mtd;

	ret = -EINVAL;
	return ret;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mtdpartctl_chrdev_open,
    .release = mtdpartctl_chrdev_release,
    .unlocked_ioctl = mtdpartctl_chrdev_ioctl,
};

static int mtdpartctl_chrdev_create(struct mtdpartctl_device *dev)
{
	int ret;

	cdev_init(&dev->ctrl_cdev, &fops);
	dev->ctrl_cdev.owner = THIS_MODULE;

	ret = cdev_add(&dev->ctrl_cdev, dev->devno, 1);
	if (ret != 0)
		goto error_cdev_add;

	return 0;

error_cdev_add:
	return ret;
}

static void mtdpartctl_chrdev_destory(struct mtdpartctl_device *dev)
{
	cdev_del(&dev->ctrl_cdev);
}

int mtdpartctl_device_create(struct mtdpartctl_device *dev)
{
	int ret;

	atomic_set(&dev->already_open, 0);

	ret = mtdpartctl_chrdev_create(dev);
	if (ret < 0)
		return ret;

	return 0;
}

void mtdpartctl_device_destroy(struct mtdpartctl_device *dev)
{
	mtdpartctl_chrdev_destory(dev);
}
