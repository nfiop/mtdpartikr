/*
 * SPDX-License-Identifier: GPL-2.0-only
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

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, argc, argv);

	INVOKE_IOCTL_WITH_RET_VARIABLE_AS_ERRNO(fd, 0xffffdddd, 0x0);

	TEST_FAIL_CONDITIONAL_IF_ERRNO_ZERO(TEST_NAME, "test1", ret);

	INVOKE_IOCTL_WITH_RET_VARIABLE_AS_ERRNO(fd, 0xffff1234, 0x33335555);

	TEST_FAIL_CONDITIONAL_IF_ERRNO_ZERO(TEST_NAME, "test2", ret);

	ret = 0;
	TEST_PASS_IF_SUCCESS_EXPECTED(TEST_NAME, ret);
}