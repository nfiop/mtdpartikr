/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "utest_helper.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#define TEST_NAME "delete_from_empty_recipe"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	/* Index 0 is not existing after opening of the device... */
	ret = mtdpartctl_recipe_del_part(fd, 0);

	TEST_PASS_IF_ERRNO_NON_ZERO_AND_EXPECTED(TEST_NAME, ret, -ENOENT);
}
