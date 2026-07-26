/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include "ctrl_dev.h"
#include "ctrl_dev_class.h"
#include "defs.h"

#define INVOKE_CALLBACK(func, dev_class, idx, mtd)                             \
	do {                                                                   \
		ret = func(dev_class, idx, mtd);                               \
		if (ret < 0)                                                   \
			return ret;                                            \
		idx++;                                                         \
	} while (0)

// FIXME: These lines are copied from drivers/mtd/mtdcore.h - maybe find a way
// to no do this...
extern struct mutex mtd_table_mutex;
extern struct mtd_info *__mtd_next_device(int i);
#define mtd_for_each_device(mtd)                                               \
	for ((mtd) = __mtd_next_device(0); (mtd) != NULL;                      \
	    (mtd) = __mtd_next_device(mtd->index + 1))

struct mtdpartctl_dev_class {
	struct class *device_class;
	struct mtd_info **mtd_devs;
	struct mtdpartctl_device **devs;
	size_t count;
	dev_t devno;

	/* This variable is used by the init path, to give an indication
	 * how many mtd devices have been ref-counted so far.
	 * On module exit, it can be used to drop ref-counts safely.
	 */
	size_t mtd_count;
};

static struct mtdpartctl_dev_class s_all_devs;

struct mtdpartctl_device *mtdpartctl_device_resolve_by_minor(size_t minor)
{
	// Minor number is simply an index to the device we want
	// so we don't anything sophisticated here.
	// This is all guaranteed by the creation seqeunce in the
	// add_devices function.
	if (minor >= s_all_devs.count)
		return NULL;
	return s_all_devs.devs[minor];
}

static void remove_devices(struct mtdpartctl_dev_class *dev_class, int max_idx)
{
	for (int idx = 0; idx < max_idx; idx++) {
		mtdpartctl_device_destroy(dev_class->devs[idx]);
	}
}

static int add_devices(struct mtdpartctl_dev_class *dev_class, int *max_idx)
{
	int ret;
	int major;
	struct mtdpartctl_device *dev;

	*max_idx = 0;
	major = MAJOR(dev_class->devno);
	for (; *max_idx < dev_class->count; ++*max_idx) {
		dev = dev_class->devs[*max_idx];
		BUG_ON(dev == NULL);

		// We don't manage the refcount of the backing MTD device here!
		// Instead, we get a refcount when getting a struct mtd_info
		// for the backing MTD device beforehand, and during removal
		// of the module we put the refcount.
		dev->mtd = dev_class->mtd_devs[*max_idx];
		BUG_ON(dev->mtd == NULL);

		dev->devno = MKDEV(major, *max_idx);
		dev->device_class = dev_class->device_class;

		ret = mtdpartctl_device_create(dev);
		if (ret != 0)
			return ret;
	}
	return 0;
}

typedef int (*handle_mtd_func_t)(
    struct mtdpartctl_dev_class *dev_class, size_t idx, struct mtd_info *);

static int mtd_for_each_master_device_filtered_locked(handle_mtd_func_t func,
    struct mtdpartctl_dev_class *dev_class, enum mtd_device_type_filter filter)
{
	BUG_ON(filter != MTD_DEVICE_TYPE_FILTER_ALL &&
	       filter != MTD_DEVICE_TYPE_FILTER_NAND &&
	       filter != MTD_DEVICE_TYPE_FILTER_NOR);
	int ret;
	size_t idx = 0;
	struct mtd_info *mtd;

	switch (filter) {
	case MTD_DEVICE_TYPE_FILTER_ALL: {
		mtd_for_each_device(mtd)
		{
			if (mtd_is_partition(mtd))
				continue;

			INVOKE_CALLBACK(func, dev_class, idx, mtd);
		}
		return 0;
	}

	case MTD_DEVICE_TYPE_FILTER_NOR: {
		mtd_for_each_device(mtd)
		{
			if (mtd_is_partition(mtd) || mtd->type != MTD_NORFLASH)
				continue;

			INVOKE_CALLBACK(func, dev_class, idx, mtd);
		}
		return 0;
	}

	case MTD_DEVICE_TYPE_FILTER_NAND: {
		mtd_for_each_device(mtd)
		{
			if (mtd_is_partition(mtd) ||
			    (mtd->type != MTD_NORFLASH &&
				mtd->type != MTD_MLCNANDFLASH))
				continue;

			INVOKE_CALLBACK(func, dev_class, idx, mtd);
		}
		return 0;
	}
	}

	return -ESRCH;
}

static int add_mtd_count(
    struct mtdpartctl_dev_class *dev_class, size_t idx, struct mtd_info *info)
{
	(void)info;
	(void)idx;
	dev_class->count++;
	return 0;
}

static int get_mtd_device_ref(
    struct mtdpartctl_dev_class *dev_class, size_t idx, struct mtd_info *info)
{
	int ret;

	/* Using get_mtd_device is unsafe - it needs to lock the mtd_table_mutex
	 * but we actually already locked it...
	 */
	ret = __get_mtd_device(info);
	if (ret < 0) {
		pr_err("mtdpartikr: failed to get ref on MTD %d, iteration idx "
		       "%zu\n",
		    info->index, idx);
		goto exit;
	}

	dev_class->mtd_devs[idx] = info;
	dev_class->mtd_count++;
	WARN_ON((idx + 1) != dev_class->mtd_count);
	ret = 0;

exit:
	return ret;
}

static void free_array_with_items(void **arr, size_t count)
{
	int idx;
	for (idx = 0; idx < count; idx++) {
		kvfree(arr[idx]);
	}
	kvfree(arr);
}

static void **alloc_array_with_items(size_t count, size_t struct_size)
{
	int idx;
	void **arr;
	arr = kvzalloc(count * sizeof(void *), GFP_KERNEL);

	if (!arr)
		return NULL;

	for (idx = 0; idx < count; idx++) {
		arr[idx] = kvzalloc(struct_size, GFP_KERNEL);
		if (!arr[idx])
			goto cleanup;
	}

	return arr;

cleanup:
	free_array_with_items(arr, idx);
	return NULL;
}

static void put_all_mtd_devices(struct mtdpartctl_dev_class *dev_class)
{
	int idx;
	WARN_ON(dev_class->mtd_count == 0);

	for (idx = 0; idx < dev_class->mtd_count; idx++) {
		put_mtd_device(dev_class->mtd_devs[idx]);
	}

	kvfree(dev_class->mtd_devs);
}

static int get_all_matching_mtd_devices(
    struct mtdpartctl_dev_class *dev_class, enum mtd_device_type_filter filter)
{
	int ret;

	dev_class->mtd_count = 0;
	dev_class->count = 0;
	mutex_lock(&mtd_table_mutex);
	/* After this call, we know the actual count of devices we want to
	 * handle.
	 */
	ret = mtd_for_each_master_device_filtered_locked(
	    add_mtd_count, dev_class, filter);
	if (ret < 0) {
		pr_warn("mtdpartikr: Failed to enumerate MTD devices\n");
		ret = -EIO;
		goto exit;
	}

	if (dev_class->count == 0) {
		pr_warn("mtdpartikr: No MTDs are available\n");
		ret = -EINVAL;
		goto exit;
	}

	dev_class->mtd_devs =
	    kvzalloc(dev_class->count * sizeof(struct mtd_info *), GFP_KERNEL);
	if (!dev_class->mtd_devs) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = mtd_for_each_master_device_filtered_locked(
	    get_mtd_device_ref, dev_class, filter);
	if (ret < 0) {
		goto free_mtd_devs_array;
	}

	ret = 0;
	goto exit;

free_mtd_devs_array:
	put_all_mtd_devices(dev_class);
	kvfree(dev_class->mtd_devs);

exit:
	mutex_unlock(&mtd_table_mutex);

	return ret;
}

static void device_class_destroy_devices(struct mtdpartctl_dev_class *dev_class)
{
	// We do these in revese to device_class_create_devices flow
	remove_devices(dev_class, dev_class->count);
	unregister_chrdev_region(dev_class->devno, dev_class->count);
}

static int device_class_create_devices(struct mtdpartctl_dev_class *dev_class)
{
	int device_idx;
	int ret;
	struct mtd_info *mtd;
	ret = alloc_chrdev_region(
	    &dev_class->devno, 0, dev_class->count, MTDPARTCTL_DEVICE_NAME);
	if (ret != 0)
		goto failed_chrdev_region_alloc;

	pr_info("mtdpartikr: registered major=%d\n", MAJOR(dev_class->devno));

	ret = add_devices(dev_class, &device_idx);
	if (ret != 0)
		goto error_create_devices;

	for (device_idx = 0; device_idx < dev_class->count; device_idx++) {
		mtd = dev_class->devs[device_idx]->mtd;
		pr_info("mtdpartikr: mtdpartctl%d => mtd%d (%s)\n", device_idx,
		    mtd->index, mtd->name);
	}

	return 0;

error_create_devices:
	remove_devices(dev_class, device_idx);
failed_chrdev_region_alloc:
	return ret; // non-zero means failure
}

int mtdpartctl_device_class_init(enum mtd_device_type_filter filter)
{
	int ret;

	ret = get_all_matching_mtd_devices(&s_all_devs, filter);
	if (ret < 0) {
		goto exit;
	}

	s_all_devs.devs = (struct mtdpartctl_device **)alloc_array_with_items(
	    s_all_devs.count, sizeof(struct mtdpartctl_device));
	if (!s_all_devs.devs) {
		ret = -ENOMEM;
		goto failed_creating_array;
	}

	s_all_devs.device_class = class_create("mtdpartctl");
	if (IS_ERR(s_all_devs.device_class)) {
		ret = PTR_ERR(s_all_devs.device_class);
		goto failed_class_create;
	}

	ret = device_class_create_devices(&s_all_devs);
	if (ret != 0)
		goto failed_creating_devices;

	return 0;

failed_creating_devices:
	class_destroy(s_all_devs.device_class);

failed_class_create:
	free_array_with_items((void **)s_all_devs.devs, s_all_devs.count);

failed_creating_array:
	put_all_mtd_devices(&s_all_devs);

exit:
	return ret;
}

void mtdpartctl_device_class_destroy(void)
{
	device_class_destroy_devices(&s_all_devs);

	class_destroy(s_all_devs.device_class);
	free_array_with_items((void **)s_all_devs.devs, s_all_devs.count);
	put_all_mtd_devices(&s_all_devs);
}
