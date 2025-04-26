#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "flashdb.h"
#include "fal_cfg.h" // FAL configuration
#include "fdb_cfg.h" // FDB configuration

#define FLASH_FILE_PATH "flash.img"


static int flash_fd = -1;
static pthread_mutex_t flash_mutex = PTHREAD_MUTEX_INITIALIZER;

/* FAL port functions: operate on flash.img */

int my_init(void) {
    flash_fd = open(FLASH_FILE_PATH, O_RDWR | O_CREAT, 0644);
    if (flash_fd < 0) {
        perror("open flash file");
        return -1;
    }
    struct stat st;
    if (fstat(flash_fd, &st) < 0) {
        perror("fstat");
        return -1;
    }
    if ((size_t)st.st_size != FLASH_SIZE) {
        if (ftruncate(flash_fd, FLASH_SIZE) < 0) {
            perror("ftruncate");
            return -1;
        }
    }
    return 0;
}

int my_read(long offset, uint8_t *buf, size_t size) {
    if (offset + size > FLASH_SIZE) return -1;
    if (lseek(flash_fd, offset, SEEK_SET) < 0) {
        perror("lseek read");
        return -1;
    }
    return (read(flash_fd, buf, size) == (ssize_t)size) ? 0 : -1;
}

int my_write(long offset, const uint8_t *buf, size_t size) {
    if (offset + size > FLASH_SIZE) return -1;
    if (lseek(flash_fd, offset, SEEK_SET) < 0) {
        perror("lseek write");
        return -1;
    }
    return (write(flash_fd, buf, size) == (ssize_t)size) ? 0 : -1;
}

int my_erase(long offset, size_t size) {
    if (offset + size > FLASH_SIZE) return -1;
    if (lseek(flash_fd, offset, SEEK_SET) < 0) {
        perror("lseek erase");
        return -1;
    }
    const size_t chunk = 4096;
    uint8_t buf_ff[chunk];
    memset(buf_ff, 0xFF, chunk);
    size_t left = size;
    while (left) {
        size_t to_write = left > chunk ? chunk : left;
        if (write(flash_fd, buf_ff, to_write) != (ssize_t)to_write) {
            perror("write erase");
            return -1;
        }
        left -= to_write;
    }
    return 0;
}

/* ====================== FAL Configuration ========================= */

/* Define a simulated flash device backed by our file */
const struct fal_flash_dev flashdev_sim = {
    .name       = "sim_flash",      /* device name */
    .addr       = 0,                 /* base offset in file */
    .len        = FLASH_SIZE,        /* total size */
    .blk_size   = BLOCK_SIZE,        /* erase block size */
    .ops        = {my_init, my_read, my_write, my_erase},
    .write_gran = 1,
};

/* ==================================================================== */


/* Lock and unlock for thread safety */
void lock(void) {
    pthread_mutex_lock(&flash_mutex);
}

void unlock(void) {
    pthread_mutex_unlock(&flash_mutex);
}

extern void kvdb_basic_sample(struct fdb_kvdb *kvdb);
extern void kvdb_type_string_sample(struct fdb_kvdb *kvdb);
extern void kvdb_type_blob_sample(struct fdb_kvdb *kvdb);
 extern void tsdb_sample(fdb_tsdb_t tsdb);

static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
/* Default KV entries */
static struct fdb_default_kv_node default_kv_table[] = {
    {"username", "armink", 0}, /* string KV */
    {"password", "123456", 0}, /* string KV */
    {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
    {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};

/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };
/* TSDB object */
struct fdb_tsdb tsdb = { 0 };
/* counts for simulated timestamp */
static int counts = 0;
static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

int main(void) {
    fdb_err_t result;

#ifdef FDB_USING_KVDB
    { /* KVDB Sample */
        struct fdb_default_kv default_kv = {
            .kvs = default_kv_table,
            .num = sizeof(default_kv_table) / sizeof(default_kv_table[0]),
        };

    //    /* Initialize simulated flash */
    //    if (fal_init() != 0) return -1;

        /* Register lock/unlock callbacks */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK,   (void *)lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);

        /* Init KVDB on partition "fdb_kvdb1" */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
        if (result != FDB_NO_ERR) {
            printf("KVDB init failed: %d\n", result);
            return -1;
        }

        /* Run sample operations */
        kvdb_basic_sample(&kvdb);
        kvdb_type_string_sample(&kvdb);
        kvdb_type_blob_sample(&kvdb);
    }
#endif /* FDB_USING_KVDB */

#ifdef FDB_USING_TSDB
    { /* TSDB Sample */
        /* set the lock and unlock function if you want */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)lock);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)unlock);
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run TSDB sample */
        tsdb_sample(&tsdb);
    }
#endif /* FDB_USING_TSDB */

    return 0;
}
