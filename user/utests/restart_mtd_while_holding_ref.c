/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define TEST_NAME "restart_mtd_while_holding_ref"

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	int mtd_fd;
	struct mtdpartctl_info info;
	char mtd_path[sizeof("/dev/mtdXXXX")];

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	ret = mtdpartctl_get_info(fd, &info);
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "get_info", ret);

	ret = snprintf(
	    mtd_path, sizeof(mtd_path), "/dev/mtd%u", info.proxy_mtd_index);
	ret = ret < 0 ? -EINVAL : 0;
	SUBTEST_FAIL_IF_ERRNO_SET(TEST_NAME, "snprintf", ret);

	mtd_fd = open(mtd_path, O_RDONLY);
	if (mtd_fd < 0) {
		TEST_FAIL_IF_REACHED(TEST_NAME, "open proxy mtd");
	}

	ret = mtdpartctl_delete_mtd_partitions(fd);
	TEST_PASS_IF_ERRNO_NON_ZERO_AND_EXPECTED(TEST_NAME, ret, -EBUSY);
}
