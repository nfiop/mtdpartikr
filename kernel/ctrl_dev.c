/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/build_bug.h>
#include <linux/fs.h>
#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/overflow.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "ctrl_dev.h"
#include "ctrl_dev_class.h"
#include "proxy_mtd_dev.h"

#include "chrdev_ioctl.h"

// FIXME: This line is copied from drivers/mtd/mtdcore.h - maybe find a way
// to no do this...
extern struct mutex mtd_table_mutex;

extern const char *const mtdpartctl_probes[];

static void restart_recipe(struct mtd_recipe *recipe)
{
	struct mtd_recipe_part *list_node, *tmp;
	mutex_lock(&recipe->lock);
	list_for_each_entry_safe(list_node, tmp, &recipe->partitions, node)
	{
		list_del(&list_node->node);
		kvfree(list_node);
	}
	recipe->parts_count = 0;
	mutex_unlock(&recipe->lock);
}

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

	/* Start with a clean recipe */
	restart_recipe(&mtdpartctl_dev->recipe);

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
    struct mtd_recipe_part *partition, u64 new_offset, u64 new_length)
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
    struct mtd_recipe *recipe, struct ext_mtd_partition_info *partition)
{
	int ret = 0;
	struct mtd_recipe_part *item;
	u64 offset = partition->base.offset;
	u64 length = partition->base.length;

	list_for_each_entry(item, &recipe->partitions, node)
	{
		if (existing_partition_contains_range(item, offset, length)) {
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}

static int verify_partition_basic_conditions(
    struct mtdpartctl_device *dev, struct mtd_partition_info *partition)
{
	struct mtd_info *master_mtd = dev->backing_mtd;
	u64 offset = partition->offset;
	u64 length = partition->length;
	u64 end;
	u64 rem;

	/* MTDPART_SIZ_FULL is defined as (0), but we should convert it
	 * on the MTDPARTCTL_IOC_ADD_MTD_PARTITION ioctl to full size, so
	 * this should not appear here...
	 */
	if (length == 0)
		return -EINVAL;

	if (check_add_overflow(offset, length, &end))
		return -EINVAL;

	if ((offset + length) > master_mtd->size) {
		pr_warn_ratelimited(
		    "mtdpartikr: failed to add new partition, (offset %llu + "
		    "len %llu) greater than MTD size %llu\n",
		    offset, length, master_mtd->size);
		return -EINVAL;
	}

	rem = do_div(offset, master_mtd->erasesize);
	if (rem != 0) {
		pr_warn_ratelimited("mtdpartikr: failed to add new partition, "
				    "offset %llu unaligned to erase size %u\n",
		    partition->offset, master_mtd->erasesize);
		return -EINVAL;
	}

	rem = do_div(length, master_mtd->erasesize);
	if (rem != 0) {
		pr_warn_ratelimited("mtdpartikr: failed to add new partition, "
				    "length %llu unaligned to erase size %u\n",
		    partition->length, master_mtd->erasesize);
		return -EINVAL;
	}

	return 0;
}

static int add_recipe_part(
    struct mtdpartctl_device *dev, struct ext_mtd_partition_info *partition)
{
	int ret;
	int name_len;
	struct mtd_recipe *recipe = &dev->recipe;
	struct mtd_recipe_part *new_part;

	ret = verify_partition_basic_conditions(dev, &partition->base);
	if (ret < 0)
		goto exit;

	mutex_lock(&recipe->lock);

	ret = verify_partition_params_locked(recipe, partition);
	if (ret < 0)
		goto unlock_recipe;

	new_part = kvzalloc(sizeof(struct mtd_recipe_part), GFP_KERNEL);
	if (!new_part) {
		ret = -ENOMEM;
		goto unlock_recipe;
	}

	/* Set the parameters of the new recipe partition now */
	new_part->offset = partition->base.offset;
	new_part->length = partition->base.length;
	name_len = strnlen(partition->base.name,
	    min(sizeof(new_part->name), sizeof(partition->base.name)));
	memcpy(new_part->name, partition->base.name, name_len);
	new_part->name[MTD_PARTITION_NAME_MAX_LENGTH - 1] = '\0';
	new_part->writable = !partition->readonly;
	new_part->powerup_lock_enabled = partition->powerup_lock_enabled;

	list_add_tail(&new_part->node, &recipe->partitions);
	recipe->parts_count++;

	ret = 0;

unlock_recipe:
	mutex_unlock(&recipe->lock);
exit:
	return ret;
}

static int del_recipe_part(struct mtdpartctl_device *dev, u32 index)
{
	int ret;
	size_t cur_idx = 0;
	struct mtd_recipe_part *item, *tmp;
	struct mtd_recipe *recipe = &dev->recipe;

	mutex_lock(&recipe->lock);

	list_for_each_entry_safe(item, tmp, &recipe->partitions, node)
	{
		if (cur_idx == index) {
			list_del(&item->node);
			kvfree(item);
			ret = 0;
			goto unlock_recipe;
		}
	}

	ret = -ENOENT;

unlock_recipe:
	mutex_unlock(&recipe->lock);
	return ret;
}

static int convert_recipe_to_mtd_partitions_locked(
    struct mtd_recipe *recipe, struct mtd_partition **partitions_np)
{
	size_t idx;
	struct mtd_recipe_part *item;
	struct mtd_partition *partition;

	/* Trying to apply an empty list is an invalid operation, don't allow it
	 * because we can't do anything meaningful about this condition.
	 */
	if (recipe->parts_count == 0) {
		return -EINVAL;
	}

	BUG_ON(list_empty(&recipe->partitions));

	*partitions_np = kvzalloc(
	    sizeof(struct mtd_partition) * recipe->parts_count, GFP_KERNEL);
	if (*partitions_np == NULL)
		return -ENOMEM;

	idx = 0;
	list_for_each_entry(item, &recipe->partitions, node)
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

static void set_proxy_mtd_about_to_respawn(
    struct mtdpartctl_device *dev, bool respawn)
{
	mutex_lock(&dev->proxy.lock);
	dev->proxy.about_to_respawn = respawn;
	mutex_unlock(&dev->proxy.lock);
}

static int respawn_proxy_mtd_locked(struct mtdpartctl_device *dev,
    struct mtd_partition *partitions, size_t partitions_count)
{
	int ret;

	set_proxy_mtd_about_to_respawn(dev, true);

	BUG_ON(dev->proxy.use_refcnt < 0);
	if (dev->proxy.use_refcnt != 0) {
		ret = -EBUSY;
		goto cleanup;
	}

	/* We should be ready to unregister, given that nobody else uses this
	 * MTD device.
	 */
	ret = mtd_device_unregister(dev->proxy.mtd);
	if (ret < 0)
		goto cleanup;

	ret = mtd_device_parse_register(dev->proxy.mtd, mtdpartctl_probes, NULL,
	    partitions, partitions_count);

cleanup:
	set_proxy_mtd_about_to_respawn(dev, false);

	return ret;
}

static int create_recipe_partitions(struct mtdpartctl_device *dev)
{
	int ret;
	struct mtd_recipe *recipe = &dev->recipe;
	struct mtd_partition *partitions = NULL;

	mutex_lock(&recipe->lock);

	ret = convert_recipe_to_mtd_partitions_locked(recipe, &partitions);
	if (ret < 0)
		goto exit;

	BUG_ON(recipe->parts_count == 0);
	ret = respawn_proxy_mtd_locked(dev, partitions, recipe->parts_count);

	kvfree(partitions);

exit:
	mutex_unlock(&recipe->lock);
	return ret;
}

static int delete_partitions(struct mtdpartctl_device *dev)
{
	int ret;
	struct mtd_recipe *recipe = &dev->recipe;

	mutex_lock(&recipe->lock);

	ret = respawn_proxy_mtd_locked(dev, NULL, 0);

	mutex_unlock(&recipe->lock);
	return ret;
}

static_assert(MTDPART_SIZ_FULL == (0));

static int adapt_possible_full_mtd_partition_length(
    struct mtdpartctl_device *dev, struct mtd_partition_info *part_info)
{
	u64 part_length = part_info->length;
	u64 mtd_dev_size = dev->backing_mtd->size;
	u64 part_offset = part_info->offset;
	/* MTDPART_SIZ_FULL should be defined as (0), but we don't like
	 * that value, so let's change it to a real length.
	 */
	if (part_length == MTDPART_SIZ_FULL) {
		if (check_sub_overflow(
			mtd_dev_size, part_offset, &part_length)) {
			return -EINVAL;
		}

		part_info->length = part_length;
	}
	return 0;
}

static int mtd_relative_master_index_to_absolute_index(
    struct mtd_info *mtd, u32 idx)
{
	u32 tmp_idx = 0;
	int absolute_idx = -1;
	struct mtd_info *child;

	BUG_ON(mtd_get_master(mtd) != mtd);

	/* NOTE: We probably don't need the locking here, because we know
	 * _almost_ for sure that nothing can't remove our partitions, but
	 * this is how the kernel does this in the `mtd_del_partition` function
	 * so we probably need to do that too.
	 */
	mutex_lock(&mtd->master.partitions_lock);
	list_for_each_entry(child, &mtd->partitions, part.node)
	{
		if (tmp_idx == idx) {
			absolute_idx = child->index;
			break;
		}
	}
	mutex_unlock(&mtd->master.partitions_lock);

	return absolute_idx;
}

static int copy_recipe_parts_list_to_user(struct mtd_recipe *recipe,
    size_t parts_count, struct recipe_partitions_list __user *arg)
{
	int ret = 0;
	u32 tmp_idx = 0;
	struct ext_mtd_partition_info __user *item =
	    (struct ext_mtd_partition_info __user
		    *)((u8 *)arg +
		       offsetof(struct recipe_partitions_list, parts));
	struct mtd_recipe_part *part;
	struct ext_mtd_partition_info info;

	mutex_lock(&recipe->lock);
	list_for_each_entry(part, &recipe->partitions, node)
	{
		if (tmp_idx >= parts_count)
			break;
		info.base.offset = part->offset;
		info.base.length = part->length;
		memcpy(
		    info.base.name, part->name, MTD_PARTITION_NAME_MAX_LENGTH);
		info.readonly = !part->writable;
		info.powerup_lock_enabled = part->powerup_lock_enabled;

		if (copy_to_user(
			item, &info, sizeof(struct ext_mtd_partition_info))) {
			ret = -EFAULT;
			break;
		}

		item++;
		tmp_idx++;
	}
	mutex_unlock(&recipe->lock);

	if (ret < 0)
		goto exit;

	/* Copy the read count into the struct so userspace knows how many
	 * partitions actually exist, if it passed a larger number than the
	 * actual partitions' count.
	 */
	if (copy_to_user(arg, &tmp_idx, sizeof(u32))) {
		ret = -EFAULT;
	}

exit:
	return ret;
}

static int copy_mtd_parts_list_to_user(struct mtd_info *mtd, size_t parts_count,
    struct mtd_partitions_list __user *arg)
{
	int ret = 0;
	u32 tmp_idx = 0;
	struct mtd_indexed_partition_info __user *item =
	    (struct mtd_indexed_partition_info __user
		    *)((u8 *)arg + offsetof(struct mtd_partitions_list, parts));
	struct mtd_info *child;
	struct mtd_indexed_partition_info info;

	BUG_ON(mtd_get_master(mtd) != mtd);

	mutex_lock(&mtd->master.partitions_lock);
	list_for_each_entry(child, &mtd->partitions, part.node)
	{
		if (tmp_idx >= parts_count)
			break;
		info.absolute_idx = child->index;
		info.offset = child->part.offset;
		info.length = child->part.size;

		if (copy_to_user(item, &info,
			sizeof(struct mtd_indexed_partition_info))) {
			ret = -EFAULT;
			break;
		}

		item++;
		tmp_idx++;
	}
	mutex_unlock(&mtd->master.partitions_lock);

	if (ret < 0)
		goto exit;

	/* Copy the read count into the struct so userspace knows how many
	 * partitions actually exist, if it passed a larger number than the
	 * actual partitions' count.
	 */
	if (copy_to_user(arg, &tmp_idx, sizeof(u32))) {
		ret = -EFAULT;
	}

exit:
	return ret;
}

static long mtdpartctl_chrdev_ioctl(
    struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct mtdpartctl_device *dev = filp->private_data;
	struct mtd_info *mtd;

	WARN_ON(dev == NULL);
	if (dev == NULL)
		return -EIO;

	mtd = dev->proxy.mtd;
	BUG_ON(mtd == NULL);

	switch (cmd) {
	case MTDPARTCTL_IOC_GET_INFO: {
		struct mtdpartctl_info tmp;
		memset(&tmp, 0, sizeof(struct mtdpartctl_info));

		/* We lock the proxy lock, so we have a coherent
		 * `proxy.mtd->index` number. This number might get stale when
		 * we finish this ioctl though, as someone might ask to respawn
		 * the proxy MTD.
		 */
		mutex_lock(&dev->proxy.lock);
		tmp.backend_mtd_index = dev->backing_mtd->index;
		tmp.proxy_mtd_index = dev->proxy.mtd->index;
		tmp.backend_mtd_size = dev->backing_mtd->size;
		tmp.erase_sector_size = dev->backing_mtd->erasesize;
		mutex_unlock(&dev->proxy.lock);

		if (copy_to_user((void __user *)arg, &tmp,
			sizeof(struct mtdpartctl_info))) {
			ret = -EFAULT;
			goto exit;
		}

		ret = 0;
		goto exit;
	}
	case MTDPARTCTL_IOC_ADD_MTD_PARTITION: {
		struct mtd_partition_info part_info;
		if (copy_from_user(&part_info, (int __user *)arg,
			sizeof(struct mtd_partition_info)))
			return -EFAULT;

		ret = adapt_possible_full_mtd_partition_length(dev, &part_info);
		if (ret < 0)
			goto exit;

		/* FIXME: It seems like up until Linux 7.2-rc4, there's simply
		 * almost no checks on the bounds of a partition. Drop this (or
		 * make these checks less restrictive) when older kernels can be
		 * ignored.
		 */
		ret = verify_partition_basic_conditions(dev, &part_info);
		if (ret < 0)
			goto exit;

		mutex_lock(&dev->proxy.lock);

		if (dev->proxy.about_to_respawn || dev->proxy.use_refcnt != 0) {
			ret = -EBUSY;
		} else {
			ret = mtd_add_partition(mtd, part_info.name,
			    part_info.offset, part_info.length);
		}

		mutex_unlock(&dev->proxy.lock);
		goto exit;
	}
	case MTDPARTCTL_IOC_DEL_MTD_PARTITION: {
		int relative_idx;
		u32 idx;
		if (copy_from_user(&idx, (int __user *)arg, sizeof(u32)))
			return -EFAULT;

		mutex_lock(&dev->proxy.lock);
		if (dev->proxy.about_to_respawn || dev->proxy.use_refcnt != 0)
			ret = -EBUSY;
		else {
			/* It should be noted that it is a technically racy
			 * thing to do, because we lock the
			 * `mtd->master.partitions_lock` mutex and release after
			 * we find the absolute index until we call
			 * `mtd_del_partition` which would lock it again.
			 * The consequence is presumably not severe though -
			 * just a fail to remove the partition.
			 */
			relative_idx =
			    mtd_relative_master_index_to_absolute_index(
				mtd, idx);
			if (relative_idx < 0)
				ret = -EINVAL;
			else
				ret = mtd_del_partition(mtd, relative_idx);
		}
		mutex_unlock(&dev->proxy.lock);
		goto exit;
	}
	case MTDPARTCTL_IOC_RECIPE_ADD_PART: {
		struct ext_mtd_partition_info partition;
		if (copy_from_user(&partition, (int __user *)arg,
			sizeof(struct ext_mtd_partition_info)))
			return -EFAULT;
		ret = add_recipe_part(dev, &partition);
		goto exit;
	}
	case MTDPARTCTL_IOC_RECIPE_DEL_PART: {
		u32 idx;
		if (copy_from_user(&idx, (int __user *)arg, sizeof(u32)))
			return -EFAULT;
		ret = del_recipe_part(dev, idx);
		goto exit;
	}
	case MTDPARTCTL_IOC_RECIPE_CREATE_PARTITIONS: {
		ret = create_recipe_partitions(dev);
		goto exit;
	}

	case MTDPARTCTL_IOC_DELETE_MTD_PARTITIONS: {
		ret = delete_partitions(dev);
		goto exit;
	}
	case MTDPARTCTL_IOC_RESTART_RECIPE: {
		restart_recipe(&dev->recipe);
		ret = 0;
		goto exit;
	}
	case MTDPARTCTL_IOC_LIST_RECIPE: {
		struct recipe_partitions_list list;
		if (copy_from_user(&list, (int __user *)arg,
			sizeof(struct recipe_partitions_list)))
			return -EFAULT;

		mutex_lock(&dev->proxy.lock);
		if (dev->proxy.about_to_respawn) {
			ret = -EBUSY;
		} else {
			ret = copy_recipe_parts_list_to_user(&dev->recipe,
			    list.read_count,
			    (struct recipe_partitions_list __user *)arg);
		}
		mutex_unlock(&dev->proxy.lock);
		goto exit;
	}
	case MTDPARTCTL_IOC_GET_PARTITION_LIST: {
		struct mtd_partitions_list list;
		if (copy_from_user(&list, (int __user *)arg,
			sizeof(struct mtd_partitions_list)))
			return -EFAULT;

		mutex_lock(&dev->proxy.lock);
		if (dev->proxy.about_to_respawn) {
			ret = -EBUSY;
		} else {
			ret = copy_mtd_parts_list_to_user(mtd, list.read_count,
			    (struct mtd_partitions_list __user *)arg);
		}
		mutex_unlock(&dev->proxy.lock);
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
	if (!try_module_get(dev->backing_mtd->owner)) {
		ret = -EIO;
		goto exit;
	}

	INIT_LIST_HEAD(&dev->recipe.partitions);
	mutex_init(&dev->recipe.lock);

	ret = proxy_mtd_create_device(dev);
	if (ret < 0)
		goto drop_module;

	ret = mtdpartctl_chrdev_create(dev);
	if (ret < 0)
		goto remove_proxy_mtd;

	dev->device = device_create(dev->device_class, NULL, dev->devno, NULL,
	    "mtdpartctl%d", MINOR(dev->devno));

	if (IS_ERR(dev->device)) {
		pr_err("mtdpartikr: device_create failed\n");
		ret = PTR_ERR(dev->device);
		goto delete_chrdev;
	}

	return 0;

delete_chrdev:
	mtdpartctl_chrdev_destory(dev);
remove_proxy_mtd:
	proxy_mtd_device_destroy(dev);
drop_module:
	module_put(dev->backing_mtd->owner);
exit:
	return ret;
}

void mtdpartctl_device_destroy(struct mtdpartctl_device *dev)
{
	device_destroy(dev->device_class, dev->devno);
	mtdpartctl_chrdev_destory(dev);
	proxy_mtd_device_destroy(dev);
	module_put(dev->backing_mtd->owner);
}
