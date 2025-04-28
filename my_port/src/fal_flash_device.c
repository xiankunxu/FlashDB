/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fal.h>

#ifdef FAL_FLASHDB_STM32F7
    int Stm32F7FlashdbRead(long offset, uint8_t *buf, size_t size);
    int Stm32F7FlashdbWrite(long offset, const uint8_t *buf, size_t size);
    int Stm32F7FlashdbErase(long offset, size_t size);

    const struct fal_flash_dev stm32f7_onchip_flash =
    {
        .name       = "stm32f7_onchip_flash",
        .addr       = 0x08000000,
        .len        = 2*1024*1024,
        .blk_size   = 256*1024,
        .ops        = {NULL, Stm32F7FlashdbRead, Stm32F7FlashdbWrite, Stm32F7FlashdbErase},
        .write_gran = 8
    };
#endif /* FAL_FLASHDB_STM32F7 */

#ifdef FAL_FLASHDB_LINUX_FILE_SIM
    int my_init(void);
    int my_read(long offset, uint8_t *buf, size_t size);
    int my_write(long offset, const uint8_t *buf, size_t size);
    int my_erase(long offset, size_t size);

    const struct fal_flash_dev flashdev_sim = {
        .name       = "sim_flash",       /* device name */
        .addr       = 2 * BLOCK_SIZE,    /* base offset in file */
        .len        = FLASH_SIZE,        /* total size */
        .blk_size   = BLOCK_SIZE,        /* erase block size */
        .ops        = {my_init, my_read, my_write, my_erase},
        .write_gran = 8,
    };
#endif /* FAL_FLASHDB_LINUX_FILE_SIM */
