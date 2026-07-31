/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "add_overlapping_context_parts"

int create_partition_info_struct(struct ext_mtd_partition_info *part_info,
    u64 offset, u64 length, long special_idx)
{
	int ret;
	memset(part_info, 0, sizeof(struct ext_mtd_partition_info));
	part_info->base.offset = offset;
	part_info->base.length = length;
	ret = snprintf(part_info->base.name, sizeof(part_info->base.name),
	    "Test partition, uptime %ld", special_idx);
	ret = ret < 0 ? -EINVAL : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", ret);

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int errno_ret;
	struct ext_mtd_partition_info part_info;
	struct sysinfo info;

	ret = sysinfo(&info);
	errno_ret = ret < 0 ? -errno : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "sysinfo", errno_ret);

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	{
		ret = create_partition_info_struct(
		    &part_info, 0, 0x000010000000 / 2, info.uptime);
		SUBTEST_FAIL_IF_ERRNO_SET(
		    TEST_NAME, "create-part1-struct", ret);

		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_CONTEXT_PART, &part_info);
		/* First partition should be added without a problem */
		SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "adding-part1", ret);
	}

	ret = create_partition_info_struct(
	    &part_info, 0, 0x000010000000, info.uptime + 1);
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "create-part2-struct", ret);

	INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
	    fd, ret, MTDPARTCTL_IOC_ADD_CONTEXT_PART, &part_info);

	TEST_PASS_IF_ERRNO_NON_ZERO_AND_EXPECTED(TEST_NAME, ret, -EINVAL);
}
