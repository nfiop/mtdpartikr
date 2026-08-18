/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "add_part_no_recipe_context"

int main(int argc, char *argv[])
{
	int fd;
	int ret;
	int errno_ret;
	struct mtd_partition_info part_info;
	struct sysinfo info;

	ret = sysinfo(&info);
	errno_ret = ret < 0 ? -errno : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "sysinfo", errno_ret);

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	memset(&part_info, 0, sizeof(struct mtd_partition_info));
	part_info.offset = 0;
	part_info.length = 0x000010000000;
	ret = snprintf(part_info.name, sizeof(part_info.name),
	    "Test partition, uptime %ld", info.uptime);
	errno_ret = ret < 0 ? -EINVAL : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", errno_ret);

	INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
	    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
