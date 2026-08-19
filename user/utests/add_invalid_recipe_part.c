/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "add_invalid_recipe_part"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int errno_ret;
	struct mtdpartctl_info mtd_info;
	struct mtd_partition_info part_info;
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

	{
		ret = (mtd_info.erase_sector_size > 1) ? 0 : -EINVAL;
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "verify_sane_erase_sector_size", ret);
	}

	memset(&part_info, 0, sizeof(struct mtd_partition_info));
	ret = snprintf(part_info.name, sizeof(part_info.name),
	    "Test partition, uptime %ld", info.uptime);
	errno_ret = ret < 0 ? -EINVAL : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", errno_ret);

	/* unaligned offset sub test */
	{
		part_info.offset = 1;
		part_info.length = mtd_info.erase_sector_size;
		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
		SUBTEST_FAIL_IF_ERRNO_UNEXPECTED(TEST_NAME,
		    "adding-partition-with-unaligned-offset", ret, -EINVAL);
	}

	/* unaligned length sub test */
	{
		part_info.offset = 0;
		part_info.length = mtd_info.erase_sector_size + 1;
		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
		SUBTEST_FAIL_IF_ERRNO_UNEXPECTED(TEST_NAME,
		    "adding-partition-with-unaligned-length", ret, -EINVAL);
	}

	/* length + offset > total_device_size sub test */
	{
		part_info.offset = 0;
		part_info.length = 0x000010000000 + mtd_info.erase_sector_size;
		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
		SUBTEST_FAIL_IF_ERRNO_UNEXPECTED(TEST_NAME,
		    "adding-partition-with-out-of-bound-size", ret, -EINVAL);
	}

	/* length + offset > total_device_size sub test */
	{
		part_info.offset = 1;
		part_info.length = UINT64_MAX;
		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
		SUBTEST_FAIL_IF_ERRNO_UNEXPECTED(TEST_NAME,
		    "adding-partition-with-overflowing-range", ret, -EINVAL);
	}

	TEST_PASS_IF_REACHED(TEST_NAME, 4);
}
