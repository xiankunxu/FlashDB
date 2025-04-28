/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define FAL_PART_HAS_TABLE_CFG  /* Common for all type of flash devices */

#ifdef __cplusplus
extern "C" {
#endif

/* target that link to flashdb library should use target_compile_definitions(flashdb PRIVATE FAL_FLASHDB_STM32F7)
 * to specify the flash device configuration. */
/* ====================== FAL_FLASHDB_STM32F7 ========================= */
#ifdef FAL_FLASHDB_STM32F7

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f2/f4)/ 32(stm32f1) */
#define FDB_WRITE_GRAN                 8

/* using KVDB feature */
#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
/* Auto update KV to latest default when current KVDB version number is changed. @see fdb_kvdb.ver_num */
/* #define FDB_KV_AUTO_UPDATE */
#endif

/* using TSDB (Time series database) feature */
// #define FDB_USING_TSDB

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev stm32f7_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &stm32f7_onchip_flash,                                                  \
}

#define FDB_KV_CACHE_TABLE_SIZE       8     /* KV cache table size, set depending on the possible maximum number of keys */
/* sector cache table size, set depending on the number of sectors for the KVDB flash. The last three sectors are used as
 * the KVDB sectors */
#define FDB_SECTOR_CACHE_TABLE_SIZE   3

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table: only KVDB for stm32f7 on chip flash. As there are only three sectors available for FlashDB, for if both
 * KVDB and TSDB there need at least four sectors. */
/* Sector9's start address 0x08140000, which has an offset relative to the flash start address 0x00140000. */
#define FAL_PART_TABLE                                                                   \
{                                                                                        \
    {FAL_PART_MAGIC_WORD,  "fdb_kvdb1",    "stm32f7_onchip_flash", 0x00140000, 3*256*1024, 0},  \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* FAL_FLASHDB_STM32F7 */
/* ====================== FAL_FLASHDB_STM32F7 ========================= */

/* ====================== FAL_FLASHDB_LINUX_FILE_SIM ========================= */
#ifdef FAL_FLASHDB_LINUX_FILE_SIM

/* the flash write granularity, unit: bit
 * only support 1(nor flash)/ 8(stm32f2/f4)/ 32(stm32f1) */
#define FDB_WRITE_GRAN                 8

/* using KVDB feature */
#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
/* Auto update KV to latest default when current KVDB version number is changed. @see fdb_kvdb.ver_num */
/* #define FDB_KV_AUTO_UPDATE */
#endif

/* using TSDB (Time series database) feature */
#define FDB_USING_TSDB

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev flashdev_sim;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &flashdev_sim,                                                  \
}

#define FDB_KV_CACHE_TABLE_SIZE       10    /* KV cache table size, set depending on the possible maximum number of keys */
#define FDB_SECTOR_CACHE_TABLE_SIZE   2     /* sector cache table size, set depending on the number of sectors for the KVDB flash */

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

#endif /* FAL_FLASHDB_LINUX_FILE_SIM */
/* ====================== FAL_FLASHDB_LINUX_FILE_SIM ========================= */

#ifdef __cplusplus
}
#endif

#endif /* _FAL_CFG_H_ */
