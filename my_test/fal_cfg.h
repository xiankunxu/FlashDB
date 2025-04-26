/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define FAL_PART_HAS_TABLE_CFG

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev flashdev_sim;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &flashdev_sim,                                                  \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FLASH_SIZE      (2 * 1024) /* 2 KiB */
#define BLOCK_SIZE      (1 * 512)  /* 512 Bytes */
#define FAL_PART_TABLE                                                                   \
{                                                                                        \
    {FAL_PART_MAGIC_WORD,  "fdb_kvdb1",    "sim_flash", 0*BLOCK_SIZE, 2*BLOCK_SIZE, 0},  \
    {FAL_PART_MAGIC_WORD,  "fdb_tsdb1",    "sim_flash", 2*BLOCK_SIZE, 2*BLOCK_SIZE, 0},  \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
