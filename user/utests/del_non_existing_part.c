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

	SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(TEST_NAME, argc, argv);

	/* Index 256 is unlikely to exist... */
	ret = mtdpartctl_delete_partition(fd, 256);

	TEST_PASS_IF_ERRNO_EXPECTED_OCCURED(TEST_NAME, ret, -EINVAL);
}