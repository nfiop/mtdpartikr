/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include "ctrl_dev.h"
#include <asm-generic/errno.h>
#include <linux/mtd/partitions.h>

#define MTD_PARSER_NAME "mtdpartikr"

const char *const mtdpartctl_probes[] = {MTD_PARSER_NAME, NULL};

static int mtdpartctl_mock_parse_fn(struct mtd_info *master,
    const struct mtd_partition **pparts, struct mtd_part_parser_data *data)
{
	/* We don't do anything deliberately - we want to invoke adding of
	 * MTD partitions with our array when calling the
	 * mtd_device_parse_register function.
	 */
	return -EOPNOTSUPP;
}

static struct mtd_part_parser mtdpartctl_mock_parser = {
    .owner = THIS_MODULE,
    .name = MTD_PARSER_NAME,
    .parse_fn = mtdpartctl_mock_parse_fn,
};

int mtdpartctl_mtd_parser_init(void)
{
	return register_mtd_parser(&mtdpartctl_mock_parser);
}

void mtdpartctl_mtd_parser_destroy(void)
{
	deregister_mtd_parser(&mtdpartctl_mock_parser);
}
