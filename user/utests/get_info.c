/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "get_info"

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	struct mtdpartctl_info info;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	ret = mtdpartctl_get_info(fd, &info);

	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
