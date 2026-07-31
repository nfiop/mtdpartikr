/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "dev.h"
#include "utest_helper.h"
#include <stdio.h>

#define TEST_NAME "del_no_existing_part"

int main(int argc, char *argv[])
{
	int ret;
	int fd;

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, fd, argc, argv);

	/* Index 1 (first partition) is likely to exist upon boot with 
	 * an `insmod nandsim ...` call.
	 */
	ret = mtdpartctl_delete_partition(fd, 1);

	TEST_PASS_IF_RET_VARIABLE_ZEROED(TEST_NAME, ret);
}
