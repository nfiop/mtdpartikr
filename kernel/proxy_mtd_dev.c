/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

#include <linux/mtd/mtd.h>
#include <linux/mutex.h>

#include "proxy_mtd_dev.h"

static void proxy_mtd_put_device(struct mtd_info *mtd)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);

	mutex_lock(&dev->proxy.lock);
	BUG_ON(dev->proxy.use_refcnt == 0);
	dev->proxy.use_refcnt--;

	mutex_unlock(&dev->proxy.lock);
}

static int proxy_mtd_get_device(struct mtd_info *mtd)
{
	int ret = 0;
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);

	mutex_lock(&dev->proxy.lock);

	if (dev->proxy.about_to_respawn)
		ret = -EBUSY;
	else
		dev->proxy.use_refcnt++;

	mutex_unlock(&dev->proxy.lock);

	return ret;
}

static int proxy_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);
	return mtd_erase(dev->backing_mtd, instr);
}

static int proxy_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
    size_t *retlen, const u_char *buf)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);
	BUG_ON(dev->backing_mtd == NULL);
	return dev->backing_mtd->_write(dev->backing_mtd, to, len, retlen, buf);
}

static int proxy_mtd_read(
    struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);
	BUG_ON(dev->backing_mtd == NULL);
	return dev->backing_mtd->_read(
	    dev->backing_mtd, from, len, retlen, buf);
}

static int proxy_mtd_write_oob(
    struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);
	BUG_ON(dev->backing_mtd == NULL);
	return dev->backing_mtd->_write_oob(dev->backing_mtd, to, ops);
}

static int proxy_mtd_read_oob(
    struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct mtdpartctl_device *dev = mtd->priv;
	BUG_ON(dev == NULL);
	BUG_ON(dev->backing_mtd == NULL);
	return dev->backing_mtd->_read_oob(dev->backing_mtd, to, ops);
}

int proxy_mtd_create_device(struct mtdpartctl_device *dev)
{
	int ret;
	struct mtd_info *backing_mtd;
	struct mtd_info *mtd;

	if (dev->backing_mtd->numeraseregions != 0) {
		pr_err("mtdpartikr: backing MTD has different erasesizes, "
		       "which we don't support currently\n");
		return -EINVAL;
	}

	dev->proxy.mtd = kvzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!dev->proxy.mtd) {
		return -ENOMEM;
	}

	mtd = dev->proxy.mtd;

	mutex_init(&dev->proxy.lock);

	backing_mtd = dev->backing_mtd;
	BUG_ON(backing_mtd == NULL);

	/* Basic identity */
	mtd->name = "mtdpartctl-proxy-mtd";
	mtd->type = backing_mtd->type;
	mtd->flags = backing_mtd->flags;
	mtd->size = backing_mtd->size;
	mtd->erasesize = backing_mtd->erasesize;

	mtd->writesize = backing_mtd->writesize;
	mtd->writebufsize = backing_mtd->writebufsize;

	mtd->oobsize = backing_mtd->oobsize;
	mtd->oobavail = backing_mtd->oobavail;

	mtd->erasesize_shift = backing_mtd->erasesize_shift;
	mtd->writesize_shift = backing_mtd->writesize_shift;
	mtd->erasesize_mask = backing_mtd->erasesize_mask;
	mtd->writesize_mask = backing_mtd->writesize_mask;

	mtd->bitflip_threshold = backing_mtd->bitflip_threshold;

	mtd->ecc_step_size = backing_mtd->ecc_step_size;
	mtd->ecc_strength = backing_mtd->ecc_strength;

	mtd->priv = dev;
	mtd->_erase = proxy_mtd_erase;

	if (backing_mtd->_write_oob) {
		BUG_ON(backing_mtd->_read_oob == NULL);
		mtd->_write_oob = proxy_mtd_write_oob;
		mtd->_read_oob = proxy_mtd_read_oob;
	} else {
		BUG_ON(backing_mtd->_read == NULL);
		BUG_ON(backing_mtd->_write == NULL);

		mtd->_write = proxy_mtd_write;
		mtd->_read = proxy_mtd_read;
	}

	/* We set _get_device and _put_device callback hooks for the proxy
	 * MTD, so we can know for sure if something uses the MTD or not.
	 */
	mtd->_get_device = proxy_mtd_get_device;
	mtd->_put_device = proxy_mtd_put_device;

	dev->proxy.use_refcnt = 0;

	ret = mtd_device_register(mtd, NULL, 0);
	if (ret != 0) {
		pr_err("ufedm: failed to register proxy MTD\n");
		kvfree(dev->proxy.mtd);
		return ret;
	}

	return 0;
}

void proxy_mtd_device_destroy(struct mtdpartctl_device *dev)
{
	mtd_device_unregister(dev->proxy.mtd);
	kvfree(dev->proxy.mtd);
}
