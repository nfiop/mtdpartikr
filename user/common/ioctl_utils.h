/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __USER__IOCTL_H_
#define __USER__IOCTL_H_

#include <errno.h>
#include <fcntl.h>

#define INVOKE_IOCTL_WITH_RET_AS_ERRNO(fd, ioctl_number, param)                \
	do {                                                                   \
		int ret;                                                       \
		ret = ioctl(fd, ioctl_number, param);                          \
		if (ret < 0) {                                                 \
			ret = -errno;                                          \
		}                                                              \
		return ret;                                                    \
	} while (0)

#define INVOKE_IOCTL_WITH_RET_VARIABLE_AS_ERRNO(fd, ioctl_number, param)       \
	do {                                                                   \
		ret = ioctl(fd, ioctl_number, param);                          \
		if (ret < 0) {                                                 \
			ret = -errno;                                          \
		}                                                              \
	} while (0)

#endif