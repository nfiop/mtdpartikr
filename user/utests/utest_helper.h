/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __UTEST_HELPER_H
#define __UTEST_HELPER_H

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dev.h"
#include "ioctl_utils.h"

#define TEST_PASS_IF_ERRNO_EXPECTED_OCCURED(test_name, ret, expected_errno)    \
	do {                                                                   \
		if (ret < 0 && ret == expected_errno) {                        \
			fprintf(stderr, "TEST PASS %s: errno %d (%s)\n",       \
			    test_name, ret, strerror(-ret));                   \
			return 0;                                              \
		}                                                              \
		fprintf(                                                       \
		    stderr, "TEST FAIL %s: unexpected success\n", test_name);  \
		return 1;                                                      \
	} while (0)

#define TEST_PASS_IF_REACHED(test_name, sub_tests_count)                       \
	do {                                                                   \
		fprintf(stderr, "TEST PASS %s: sub-tests passed - %d\n",       \
		    test_name, sub_tests_count);                               \
		return 0;                                                      \
	} while (0)

#define TEST_PASS_IF_SUCCESS_EXPECTED(test_name, ret)                          \
	do {                                                                   \
		if (ret == 0) {                                                \
			fprintf(stderr, "TEST PASS %s: ret 0\n", test_name);   \
			return 0;                                              \
		}                                                              \
		fprintf(stderr, "TEST FAIL %s: unexpected errno %d (%s)\n",    \
		    test_name, ret, strerror(-ret));                           \
		return 1;                                                      \
	} while (0)

#define TEST_FAIL_CONDITIONAL_IF_ERRNO_SET(test_name, sub_reason, ret)         \
	do {                                                                   \
		if (ret < 0) {                                                 \
			fprintf(stderr, "TEST FAIL %s (%s): errno %d (%s)\n",  \
			    test_name, sub_reason ? sub_reason : "n/a", ret,   \
			    strerror(-ret));                                   \
			return 1;                                              \
		}                                                              \
	} while (0)

#define TEST_FAIL_CONDITIONAL_IF_ERRNO_ZERO(test_name, sub_reason, ret)        \
	do {                                                                   \
		if (ret == 0) {                                                \
			fprintf(stderr,                                        \
			    "TEST FAIL %s (%s): unexpected success\n",         \
			    test_name, sub_reason ? sub_reason : "n/a");       \
			return 1;                                              \
		}                                                              \
	} while (0)

#define SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(test_name, argc, argv)           \
	do {                                                                   \
		fd = get_mtdpartctl_device_fd(argc, argv);                     \
		if (fd < 0) {                                                  \
			fprintf(stderr,                                        \
			    "TEST FAIL %s: failed to open device\n",           \
			    test_name);                                        \
			return 1;                                              \
		}                                                              \
	} while (0)

static inline int get_mtdpartctl_device_fd(int argc, char *argv[])
{
	int fd;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <index>\n", argv[0]);
		return -1;
	}

	fd = open_mtdpartctl_device_by_argv_index(argv[1]);
	if (fd < 0) {
		return -1;
	}

	return fd;
}

#endif
