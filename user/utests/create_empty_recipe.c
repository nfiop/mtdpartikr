/*
 * SPDX-License-Identifier: GPL-2.0-only OR MIT
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "create_empty_recipe"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	/* After opening a file descriptor of `mtdpartctl` device
	 * it should have empty recipe context.
	 */
	ret = mtdpartctl_recipe_create_partitions(fd);

	TEST_PASS_IF_ERRNO_NON_ZERO_AND_EXPECTED(TEST_NAME, ret, -EINVAL);
}
