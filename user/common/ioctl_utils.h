/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __USER__IOCTL_H_
#define __USER__IOCTL_H_

#include <errno.h>
#include <fcntl.h>

#define INVOKE_IOCTL_WITH_RET_AS_ERRNO(__fd, __ioctl_num, __param)             \
	do {                                                                   \
		int __ret;                                                     \
		__ret = ioctl(__fd, __ioctl_num, __param);                     \
		if (__ret < 0) {                                               \
			__ret = -errno;                                        \
		}                                                              \
		return __ret;                                                  \
	} while (0)

#define INVOKE_IOCTL_WITH_VARIABLE_AS_ERRNO(                                   \
    __fd, __ret_var, __ioctl_num, __param)                                     \
	do {                                                                   \
		__ret_var = ioctl(__fd, __ioctl_num, __param);                 \
		if (__ret_var < 0) {                                           \
			__ret_var = -errno;                                    \
		}                                                              \
	} while (0)

#endif
