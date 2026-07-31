/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "create_empty_context"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	/* After opening a file descriptor of `mtdpartctl` device
	 * it should have empty context.
	 */
	ret = mtdpartctl_context_create_partitions(fd);

	TEST_PASS_IF_ERRNO_OCCURED(TEST_NAME, ret, -EINVAL);
}
