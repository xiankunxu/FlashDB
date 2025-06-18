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

#define FLASH_FILE_PATH "flash.img"

static int flash_fd = -1;
static pthread_mutex_t flash_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ==================================================================== */
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
    if ((size_t)st.st_size != flashdev_sim.addr + flashdev_sim.len) {
        if (ftruncate(flash_fd, flashdev_sim.addr + flashdev_sim.len) < 0) {
            perror("ftruncate");
            return -1;
        }
    }
    return 0;
}

int my_read(long offset, uint8_t *buf, size_t size) {
    if (offset + size > flashdev_sim.len) return -1;
    if (lseek(flash_fd, flashdev_sim.addr + offset, SEEK_SET) < 0) {
        perror("lseek read");
        return -1;
    }
    return (read(flash_fd, buf, size) == (ssize_t)size) ? 0 : -1;
}

int my_write(long offset, const uint8_t *buf, size_t size) {
    if (offset + size > flashdev_sim.len) return -1;
    if (lseek(flash_fd, flashdev_sim.addr + offset, SEEK_SET) < 0) {
        perror("lseek write");
        return -1;
    }
    return (write(flash_fd, buf, size) == (ssize_t)size) ? 0 : -1;
}

int my_erase(long offset, size_t size) {
    if (offset + size > flashdev_sim.len) return -1;
    if (lseek(flash_fd, flashdev_sim.addr + offset, SEEK_SET) < 0) {
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

static bool my_query_cb(fdb_tsl_t tsl, void *arg) {
    if (tsl->log_len == 8) {
        return false;
    }

    struct fdb_blob blob;
    fdb_tsdb_t db = arg;
    uint8_t buf[tsl->log_len];

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, buf, tsl->log_len)));
    printf("[%u] %s\n", (unsigned)tsl->time, (char*)buf);
    return false;
}

static bool oldest_tsl_cb(fdb_tsl_t tsl, void *arg) {
    struct fdb_blob blob;
    fdb_tsdb_t db = arg;
    uint8_t buf[tsl->log_len];

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, buf, tsl->log_len)));
    printf("oldest time: [%u], log: %s\n", (unsigned)tsl->time, (char*)buf);
    return true;
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
//        tsdb_sample(&tsdb);

        /* Append a log entry */
        struct fdb_blob blob;
        char log_data[128];
        snprintf(log_data, sizeof(log_data), "Log entry at time %d", counts + 1);
        fdb_blob_make(&blob, log_data, strlen(log_data) + 1); // +1 for null terminator
        result = fdb_tsl_append(&tsdb, &blob);
        if (result != FDB_NO_ERR) {
            printf("TSDB append failed: %d\n", result);
            return -1;
        }
        printf("TSDB append succeeded.\n");
        /* Query the TSDB by time */

        printf("Querying TSDB entries by iterating============:\n");
        fdb_tsl_iter(&tsdb, my_query_cb, &tsdb);
        
        printf("Querying TSDB entries by time=================:\n");
        time_t from_time = 1, to_time = counts;
        fdb_tsl_iter_by_time(&tsdb, from_time, to_time, my_query_cb, &tsdb);

        printf("Get oldest TSDB entry =================:\n");
        from_time = 0;
        to_time = counts;
        fdb_tsl_iter_by_time(&tsdb, from_time, to_time, oldest_tsl_cb, &tsdb);
    }
#endif /* FDB_USING_TSDB */

    return 0;
}
