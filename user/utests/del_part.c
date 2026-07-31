/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "del_part"

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	struct mtd_partition_info part_info;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	{
		memset(&part_info, 0, sizeof(struct mtd_partition_info));
		part_info.offset = 0;
		part_info.length = 0x000010000000;
		ret = snprintf(
		    part_info.name, sizeof(part_info.name), "Test partition");
		ret = ret < 0 ? -EINVAL : 0;
		SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", ret);

		INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(
		    fd, ret, MTDPARTCTL_IOC_ADD_MTD_PARTITION, &part_info);
		SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "adding-partition", ret);
	}

	/* Index 0 should exist after previous call is returning success.
	 */
	ret = mtdpartctl_delete_partition(fd, 0);

	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
