/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "ctrl_dev.h"
#include "ctrl_dev_class.h"
#include "defs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liav A");
MODULE_DESCRIPTION("MTD partitioning in kernel runtime");
MODULE_VERSION("0.1");

static char *device_type_filter;

module_param(device_type_filter, charp, 0444);
MODULE_PARM_DESC(
    device_type_filter, "Device type filter (nand,nor,all). Default=all");

static int determine_device_type_filter(enum mtd_device_type_filter *filter)
{
	int ret;
	if (!device_type_filter || !strcmp(device_type_filter, "all")) {
		ret = 0;
		*filter = MTD_DEVICE_TYPE_FILTER_ALL;
		goto exit;
	}

	if (!strcmp(device_type_filter, "nand")) {
		ret = 0;
		*filter = MTD_DEVICE_TYPE_FILTER_NAND;
		goto exit;
	}

	if (!strcmp(device_type_filter, "nor")) {
		ret = 0;
		*filter = MTD_DEVICE_TYPE_FILTER_NOR;
		goto exit;
	}

	pr_err("mtdpartikr: invalid device type filter %s", device_type_filter);
	ret = -EINVAL;
exit:
	return ret;
}

static int __init mtdpartikr_init(void)
{
	int ret;
	enum mtd_device_type_filter filter;

	ret = determine_device_type_filter(&filter);
	if (ret < 0)
		goto exit;

	ret = mtdpartctl_device_class_init(filter);
	if (ret < 0)
		goto exit;

	printk(KERN_INFO "mtdpartikr: kernel module loaded!\n");
	return 0;

exit:
	return ret;
}

static void __exit mtdpartikr_exit(void)
{
	mtdpartctl_device_class_destroy();
	printk(KERN_INFO "mtdpartikr: kernel module unloaded!\n");
}

module_init(mtdpartikr_init);
module_exit(mtdpartikr_exit);
