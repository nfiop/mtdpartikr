/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __DEFS_H
#define __DEFS_H

#ifndef __KERNEL__
#include <stdint.h> /* uint32_t, uint64_t */
#include <sys/types.h>
#endif

#include <linux/types.h>

#ifndef __KERNEL__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

#define MTD_PARTITION_NAME_MAX_LENGTH 128

enum mtd_device_type_filter {
	MTD_DEVICE_TYPE_FILTER_ALL,
	MTD_DEVICE_TYPE_FILTER_NAND,
	MTD_DEVICE_TYPE_FILTER_NOR,
};

#endif
