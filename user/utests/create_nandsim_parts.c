/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "create_nandsim_parts"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int errno_ret;
	struct mtdpartctl_info mtd_info;
	struct ext_mtd_partition_info part_info;
	struct sysinfo info;

	ret = sysinfo(&info);
	errno_ret = ret < 0 ? -errno : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "sysinfo", errno_ret);

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	{
		ret = mtdpartctl_get_info(fd, &mtd_info);
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "mtdpartctl_get_info", ret);
	}

	/* Sanity check - erase_sector_size > 1 */
	{
		ret = (mtd_info.erase_sector_size > 1) ? 0 : -EINVAL;
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "verify_sane_erase_sector_size", ret);
	}

	memset(&part_info, 0, sizeof(struct ext_mtd_partition_info));

	/* Create 20 partitions with size of an erase block each */
	for (int i = 0; i < 20; i++) {
		ret = snprintf(part_info.base.name, sizeof(part_info.base.name),
		    "Test partition %d", i);
		errno_ret = ret < 0 ? -EINVAL : 0;
		SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", errno_ret);

		part_info.base.offset = i * mtd_info.erase_sector_size;
		part_info.base.length = mtd_info.erase_sector_size;
		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_RECIPE_ADD_PART, &part_info);
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "adding-partition-via-recipe-context", ret);
	}

	ret = mtdpartctl_recipe_create_partitions(fd);
	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
