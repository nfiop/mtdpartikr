/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __MTDPARTCTL_DEVICE_CLASS
#define __MTDPARTCTL_DEVICE_CLASS

#include "defs.h"

struct mtdpartctl_device *mtdpartctl_device_resolve_by_minor(size_t minor);
int mtdpartctl_device_class_init(enum mtd_device_type_filter filter);
void mtdpartctl_device_class_destroy(void);

#endif
