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

static inline int test_pass_if_errno_non_zero_and_expected(
    const char *test_name, int ret, int expected_errno)
{
	if (ret < 0) {
		if (ret == expected_errno) {
			fprintf(stderr, "TEST PASS %s: errno %d (%s)\n",
			    test_name, ret, strerror(-ret));
			return 0;
		}
		fprintf(stderr,
		    "TEST FAIL %s: unexpected errno %d (%s), expected %d\n",
		    test_name, ret, strerror(-ret), expected_errno);
		return 1;
	}
	fprintf(stderr, "TEST FAIL %s: unexpected success\n", test_name);
	return 1;
}

#define TEST_PASS_IF_ERRNO_NON_ZERO_AND_EXPECTED(                              \
    __test_name, __ret_var, __expected_errno)                                  \
	do {                                                                   \
		return test_pass_if_errno_non_zero_and_expected(               \
		    __test_name, __ret_var, __expected_errno);                 \
	} while (0)

static inline int test_pass_if_reached(
    const char *test_name, size_t sub_tests_count)
{
	fprintf(stderr, "TEST PASS %s: sub-tests passed - %zu\n", test_name,
	    sub_tests_count);
	return 0;
}

#define TEST_PASS_IF_REACHED(__test_name, __sub_tests_count)                   \
	do {                                                                   \
		return test_pass_if_reached(__test_name, __sub_tests_count);   \
	} while (0)

static inline int test_fail_if_reached(
    const char *test_name, const char *reason)
{
	fprintf(stderr, "TEST FAIL %s: %s failed\n", test_name, reason);
	return 1;
}

#define TEST_FAIL_IF_REACHED(__test_name, __reason)                            \
	do {                                                                   \
		return test_fail_if_reached(__test_name, __reason);            \
	} while (0)

static inline int test_pass_if_ret_variable_is_zero(
    const char *test_name, int ret)
{
	if (ret == 0) {
		fprintf(stderr, "TEST PASS %s: return value is 0\n", test_name);
		return 0;
	}
	fprintf(stderr, "TEST FAIL %s: unexpected errno %d (%s)\n", test_name,
	    ret, strerror(-ret));
	return 1;
}

#define TEST_PASS_IF_RET_VARIABLE_ZEROED(__test_name, __ret_var)               \
	do {                                                                   \
		return test_pass_if_ret_variable_is_zero(                      \
		    __test_name, __ret_var);                                   \
	} while (0)

static inline int subtest_fail_if_errno_set(
    const char *test_name, const char *sub_reason, int ret)
{
	if (ret < 0) {
		fprintf(stderr, "TEST FAIL %s (%s): errno %d (%s)\n", test_name,
		    sub_reason ? sub_reason : "n/a", ret, strerror(-ret));
		return 1;
	}
	return 0;
}

#define SUBTEST_FAIL_IF_ERRNO_SET(__test_name, __sub_reason, __ret_var)        \
	do {                                                                   \
		int __ret;                                                     \
		__ret = subtest_fail_if_errno_set(                             \
		    __test_name, __sub_reason, __ret_var);                     \
		if (__ret)                                                     \
			return 1;                                              \
	} while (0)

static inline int subtest_fail_if_errno_set_unexpected(
    const char *test_name, const char *sub_reason, int ret, int expected_errno)
{
	if (ret != expected_errno) {
		fprintf(stderr,
		    "TEST FAIL %s (%s): unexpected errno %d, expected %s\n",
		    test_name, sub_reason ? sub_reason : "n/a", ret,
		    strerror(-expected_errno));
		return 1;
	}
	return 0;
}

#define SUBTEST_FAIL_IF_ERRNO_UNEXPECTED(                                      \
    __test_name, __sub_reason, __ret_var, __expected_errno)                    \
	do {                                                                   \
		int __ret;                                                     \
		__ret = subtest_fail_if_errno_set_unexpected(                  \
		    __test_name, __sub_reason, __ret_var, __expected_errno);   \
		if (__ret)                                                     \
			return 1;                                              \
	} while (0)

static inline int subtest_fail_if_errno_zero(
    const char *test_name, const char *sub_reason, int ret)
{
	if (ret == 0) {
		fprintf(stderr, "TEST FAIL %s (%s): unexpected success\n",
		    test_name, sub_reason ? sub_reason : "n/a");
		return 1;
	}
	return 0;
}

#define SUBTEST_FAIL_IF_ERRNO_ZERO(__test_name, __sub_reason, __ret_var)       \
	do {                                                                   \
		int __ret;                                                     \
		__ret = subtest_fail_if_errno_zero(                            \
		    __test_name, __sub_reason, __ret_var);                     \
		if (__ret)                                                     \
			return 1;                                              \
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

static inline int open_mtdpartctl_device_or_exit(
    const char *test_name, int argc, char **argv)
{
	int fd;
	fd = get_mtdpartctl_device_fd(argc, argv);
	if (fd < 0) {
		fprintf(
		    stderr, "TEST FAIL %s: failed to open device\n", test_name);
		exit(1);
	}
	return fd;
}

#define SET_FD_WITH_MTDPARTCTL_DEVICE_OR_FAIL(                                 \
    __test_name, __fd_ret, __argc, __argv)                                     \
	do {                                                                   \
		__fd_ret = open_mtdpartctl_device_or_exit(                     \
		    __test_name, __argc, __argv);                              \
	} while (0)

#endif
