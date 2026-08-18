/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "delete_recipe_part"

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	int errno_ret;
	struct mtdpartctl_info mtd_info;
	struct ext_mtd_partition_info part_info;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	{
		ret = mtdpartctl_get_info(fd, &mtd_info);
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "mtdpartctl_get_info", ret);
	}

	ret = snprintf(part_info.base.name, sizeof(part_info.base.name),
	    "Test partition 0");
	errno_ret = ret < 0 ? -EINVAL : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", errno_ret);

	part_info.base.offset = mtd_info.erase_sector_size;
	part_info.base.length = mtd_info.erase_sector_size;
	INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
	    fd, ret, MTDPARTCTL_IOC_RECIPE_ADD_PART, &part_info);
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "adding-recipe-part", ret);

	ret = mtdpartctl_recipe_del_part(fd, 0);

	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "recipe-del-part", ret);

	TEST_PASS_IF_REACHED(TEST_NAME, 2);
}
