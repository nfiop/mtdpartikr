/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#ifndef __PROXY_MTD_DEVICE__
#define __PROXY_MTD_DEVICE__

#include <linux/atomic.h>
#include <linux/types.h>

#include "ctrl_dev.h"

int proxy_mtd_create_device(struct mtdpartctl_device *dev);
void proxy_mtd_device_destroy(struct mtdpartctl_device *dev);

#endif
