/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/overflow.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "ctrl_dev.h"
#include "ctrl_dev_class.h"

#include "chrdev_ioctl.h"

// FIXME: This line is copied from drivers/mtd/mtdcore.h - maybe find a way
// to no do this...
extern struct mutex mtd_table_mutex;

extern const char *const mtdpartctl_probes[];

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

static int existing_partition_contains_range(
    struct mtdparser_partition *partition, u64 new_offset, u64 new_length)
{
	u64 existing_partition_end;
	u64 new_partition_end;

	bool would_overflow = check_add_overflow(
	    partition->offset, partition->length, &existing_partition_end);
	BUG_ON(would_overflow);

	would_overflow =
	    check_add_overflow(new_offset, new_length, &new_partition_end);
	/* We checked for overflow in verify_partition_basic_conditions function
	 */
	BUG_ON(would_overflow);

	return new_offset < existing_partition_end &&
	       partition->offset < new_partition_end;
}

static int verify_partition_params_locked(
    struct mtdparser_context *context, struct ext_mtd_partition_info *partition)
{
	int ret = 0;
	struct mtdparser_partition *item;
	u64 offset = partition->base.offset;
	u64 length = partition->base.length;

	list_for_each_entry(item, &context->partitions, node)
	{
		if (existing_partition_contains_range(item, offset, length)) {
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}

static void restart_parser_context(struct mtdparser_context *context)
{
	struct mtdparser_partition *list_node, *tmp;
	mutex_lock(&context->lock);
	list_for_each_entry_safe(list_node, tmp, &context->partitions, node)
	{
		list_del(&list_node->node);
		kvfree(list_node);
	}
	context->count = 0;
	mutex_unlock(&context->lock);
}

static int verify_partition_basic_conditions(
    struct mtdpartctl_device *dev, struct ext_mtd_partition_info *partition)
{
	struct mtd_info *master_mtd = dev->mtd;
	u64 offset = partition->base.offset;
	u64 length = partition->base.length;
	u64 end;

	if (check_add_overflow(offset, length, &end))
		return -EINVAL;

	if ((offset + length) > master_mtd->size) {
		pr_warn_ratelimited(
		    "mtdpartikr: failed to add new partition, (off %llu + "
		    "len %llu) greater than MTD size %llu\n",
		    offset, length, master_mtd->size);
		return -EINVAL;
	}

	if ((offset % dev->mtd->erasesize) != 0) {
		pr_warn_ratelimited("mtdpartikr: failed to add new partition, "
				    "offset %llu unaligned to erase size %u\n",
		    offset, master_mtd->erasesize);
		return -EINVAL;
	}

	if ((length % dev->mtd->erasesize) != 0) {
		pr_warn_ratelimited("mtdpartikr: failed to add new partition, "
				    "length %llu unaligned to erase size %u\n",
		    length, master_mtd->erasesize);
		return -EINVAL;
	}

	return 0;
}

static int add_parser_partition(
    struct mtdpartctl_device *dev, struct ext_mtd_partition_info *partition)
{
	int ret;
	struct mtdparser_context *context = &dev->parser_context;
	struct mtdparser_partition *new_part;

	ret = verify_partition_basic_conditions(dev, partition);
	if (ret < 0)
		goto exit;

	mutex_lock(&context->lock);

	ret = verify_partition_params_locked(context, partition);
	if (ret < 0)
		goto unlock_context;

	new_part = kvzalloc(sizeof(struct mtdparser_partition), GFP_KERNEL);
	if (!new_part) {
		ret = -ENOMEM;
		goto unlock_context;
	}

	list_add(&new_part->node, &context->partitions);
	context->count++;

	ret = 0;

unlock_context:
	mutex_unlock(&context->lock);
exit:
	return ret;
}

static void get_mtd_device_after_put(struct mtd_info *mtd)
{
	int ret;
	mutex_lock(&mtd_table_mutex);

	ret = __get_mtd_device(mtd);

	/* We should hold a refcount on the module so this should be not
	 * possible to fail.
	 */
	BUG_ON(ret != 0);

	mutex_unlock(&mtd_table_mutex);
}

static int convert_context_to_mtd_partitions_locked(
    struct mtdparser_context *context, struct mtd_partition **partitions_np)
{
	size_t idx;
	struct mtdparser_partition *item;
	struct mtd_partition *partition;

	/* Trying to apply an empty list is an invalid operation, don't allow it
	 * because we can't do anything meaningful about this condition.
	 */
	if (context->count == 0) {
		return -EINVAL;
	}

	BUG_ON(list_empty(&context->partitions));

	*partitions_np =
	    kvzalloc(sizeof(struct mtd_partition) * context->count, GFP_KERNEL);
	if (*partitions_np == NULL)
		return -ENOMEM;

	idx = 0;
	list_for_each_entry(item, &context->partitions, node)
	{
		partition = &(*partitions_np)[idx];
		partition->name = item->name;
		partition->types = mtdpartctl_probes;
		partition->offset = item->offset;
		partition->size = item->length;
		if (!item->writable)
			partition->mask_flags |= MTD_WRITEABLE;
		if (item->powerup_lock_enabled)
			partition->add_flags |= MTD_POWERUP_LOCK;
		idx++;
	}

	return 0;
}

static int apply_parser(struct mtdpartctl_device *dev)
{
	int ret;
	struct mtdparser_context *context = &dev->parser_context;
	struct mtd_partition *partitions = NULL;

	mutex_lock(&context->lock);

	ret = convert_context_to_mtd_partitions_locked(context, &partitions);
	if (ret < 0)
		goto exit;

	/* NOTE!: We hold a refcount on the MTD device as well as the module
	 * that owns the MTD device. We will drop the refcount on the MTD
	 * device, but keep the ref on the module to ensure the device doesn't
	 * disappear.
	 * This is quite tricky and playing with fire, but there's no way around
	 * when we want to re-initialize the MTD device with new partitions.
	 */

	put_mtd_device(dev->mtd);

	/* We should be ready to unregister, given that nobody else uses this
	 * MTD device.
	 */
	ret = mtd_device_unregister(dev->mtd);
	if (ret < 0)
		goto get_mtd_device_back;

	BUG_ON(context->count == 0);
	ret = mtd_device_parse_register(
	    dev->mtd, mtdpartctl_probes, NULL, partitions, context->count);

get_mtd_device_back:
	get_mtd_device_after_put(dev->mtd);

exit:
	mutex_unlock(&context->lock);
	return ret;
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
	BUG_ON(backend == NULL);

	switch (cmd) {
	case MTDPARTCTL_IOC_GET_INFO: {
		struct mtdpartctl_info tmp;
		memset(&tmp, 0, sizeof(struct mtdpartctl_info));
		tmp.backend_mtd_index = dev->mtd->index;
		tmp.erase_sector_size = dev->mtd->erasesize;

		if (copy_to_user((void __user *)arg, &tmp,
			sizeof(struct mtdpartctl_info))) {
			ret = -EFAULT;
			goto exit;
		}

		ret = 0;
		goto exit;
	}
	case MTDPARTCTL_IOC_ADD_NEW_PART: {
		// FIXME: We don't validate offsets or lengths like we do for
		// the parser functionality. Is it a problem or we are OK
		// relying on the MTD API?
		struct mtd_partition_info part_info;
		if (copy_from_user(&part_info, (int __user *)arg,
			sizeof(struct mtd_partition_info)))
			return -EFAULT;
		ret = mtd_add_partition(backend, part_info.name,
		    part_info.offset, part_info.length);
		goto exit;
	}
	case MTDPARTCTL_IOC_DEL_PART: {
		u32 idx;
		if (copy_from_user(&idx, (int __user *)arg, sizeof(u32)))
			return -EFAULT;
		ret = mtd_del_partition(backend, idx);
		goto exit;
	}
	case MTDPARTCTL_IOC_ADD_PARSER_PART: {
		struct ext_mtd_partition_info partition;
		if (copy_from_user(&partition, (int __user *)arg,
			sizeof(struct ext_mtd_partition_info)))
			return -EFAULT;
		ret = add_parser_partition(dev, &partition);
		goto exit;
	}
	case MTDPARTCTL_IOC_APPLY_PARSER: {
		ret = apply_parser(dev);
		goto exit;
	}
	case MTDPARTCTL_IOC_RESTART_PARSER: {
		restart_parser_context(&dev->parser_context);
		ret = 0;
		goto exit;
	}

	default:
		ret = -EINVAL;
		goto exit;
	}

exit:
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

	/* Get a refcount on the owner of the MTD device -
	 * this way we ensure that the MTD device can't disappear
	 * when we unregister and register it when "parsing" the partition
	 * table on that MTD device.
	 */
	if (!try_module_get(dev->mtd->owner)) {
		ret = -EIO;
		goto exit;
	}

	INIT_LIST_HEAD(&dev->parser_context.partitions);
	mutex_init(&dev->parser_context.lock);

	ret = mtdpartctl_chrdev_create(dev);
	if (ret < 0)
		goto drop_module;

	dev->device = device_create(dev->device_class, NULL, dev->devno, NULL,
	    "mtdpartctl%d", MINOR(dev->devno));

	if (IS_ERR(dev->device)) {
		pr_err("mtdpartikr: device_create failed\n");
		ret = PTR_ERR(dev->device);
		goto delete_chrdev;
	}

	return 0;

drop_module:
	module_put(dev->mtd->owner);

delete_chrdev:
	mtdpartctl_chrdev_destory(dev);

exit:
	return ret;
}

void mtdpartctl_device_destroy(struct mtdpartctl_device *dev)
{
	device_destroy(dev->device_class, dev->devno);
	mtdpartctl_chrdev_destory(dev);
	module_put(dev->mtd->owner);
}
