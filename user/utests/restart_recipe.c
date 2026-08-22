/*
 * SPDX-License-Identifier: GPL-2.0-only OR MIT
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "restart_recipe"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	ret = mtdpartctl_recipe_restart(fd);

	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
