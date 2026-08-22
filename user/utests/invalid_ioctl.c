/*
 * SPDX-License-Identifier: GPL-2.0-only OR MIT
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "invalid_ioctl"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(fd, ret, 0xffffdddd, 0x0);

	SUBTEST_FAIL_IF_ERRNO_ZERO(TEST_NAME, "test1", ret);

	INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(fd, ret, 0xffff1234, 0x33335555);

	SUBTEST_FAIL_IF_ERRNO_ZERO(TEST_NAME, "test2", ret);

	TEST_PASS_IF_REACHED(TEST_NAME, 2);
}
