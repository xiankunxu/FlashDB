#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <pybind11/pybind11.h>
#include "flashdb.h"

#define FLASH_FILE_PATH "flash_lib.img"

static int flash_fd = -1;
static pthread_mutex_t flash_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ==================================================================== */

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

/* Even though we have defined const struct fal_flash_dev flashdev_sim in the fal_flash_device.c. Here we still have to define
 * this file's own flashdev_sim (in main.cpp it is not necessary). The reason is that pybind11 will always generate shared library.
 * If don't define its own flashde_sim, will have linking error: relocation R_X86_64_PC32 against symbol `flashdev_sim' can not be
 * used when making a shared object */
const struct fal_flash_dev flashdev_sim = {
    .name       = "sim_flash",       /* device name */
    .addr       = 2 * BLOCK_SIZE,    /* base offset in file */
    .len        = FLASH_SIZE,        /* total size */
    .blk_size   = BLOCK_SIZE,        /* erase block size */
    .ops        = {my_init, my_read, my_write, my_erase},
    .write_gran = 8,
};

static int counts = 0;
static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

struct env_status {
    int temp;
    int humi;
};

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct env_status status;
    fdb_tsdb_t db = static_cast<fdb_tsdb_t>(arg);

    fdb_blob_read((fdb_db_t) db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    printf("[query_cb] queried a TSL: time: %d, temp: %d, humi: %d\n", tsl->time, status.temp, status.humi);

    return false;
}

namespace py = pybind11;
PYBIND11_MODULE(flashdb_test_lib, m) {
    m.doc() = "FlashDB test library"; // Optional module docstring

    py::class_<struct env_status>(m, "env_status")
        .def(py::init<>())
        .def_readwrite("temp", &env_status::temp)
        .def_readwrite("humi", &env_status::humi);
    
    py::enum_<fdb_err_t>(m, "FDBErr", py::arithmetic())
        .value("FDB_NO_ERR",      FDB_NO_ERR)
        .value("FDB_ERASE_ERR",   FDB_ERASE_ERR)
        .value("FDB_READ_ERR",    FDB_READ_ERR)
        .value("FDB_WRITE_ERR",   FDB_WRITE_ERR)
        .value("FDB_PART_NOT_FOUND",  FDB_PART_NOT_FOUND)
        .value("FDB_KV_NAME_ERR",  FDB_KV_NAME_ERR)
        .value("FDB_KV_NAME_EXIST",  FDB_KV_NAME_EXIST)
        .value("FDB_SAVED_FULL",  FDB_SAVED_FULL)
        .value("FDB_INIT_FAILED", FDB_INIT_FAILED)
        .export_values();   // optional: let Python lookups like ImageState.FACTORY

    py::class_<struct fdb_kvdb>(m, "fdb_kvdb")
        .def(py::init<>());

    py::class_<struct fdb_tsdb>(m, "fdb_tsdb")
        .def(py::init<>());
    
    py::class_<struct fdb_blob>(m, "fdb_blob")
        .def(py::init<>());
    
    py::class_<struct fdb_default_kv>(m, "fdb_default_kv")
        .def(py::init<>());

    /* fdb_kvdb_init takes struct fdb_default_kv* as one of its parameter. So first need to
     * expose struct fdb_default_kv by the above py::class_ command. Then on python side can
     * either pass a fdb_default_kv object(which will automatically convert to pointer by pybind11),
     * or None (which automatically convert to NULL) */
    m.def("fdb_kvdb_init", 
        /* Define a lambda instead of directly &fdb_kvdb_init is because the return type fdb_err_t is
         * not able to be interpreted by Python, so use a wrapper to return int. Use py::enum_<fdb_err_t>
         * is another way so Python can understand what fdb_err_t is. */
        [](fdb_kvdb_t kvdb,
           const char *name,
           const char *path,
           fdb_default_kv *default_kv,
           void *user_data) -> int
        {
            // call the real C function, cast its result to int
            return static_cast<int>(
                fdb_kvdb_init(kvdb, name, path, default_kv, user_data)
            );
        },
        "Initialize KVDB",
        py::arg("kvdb"), py::arg("name"), py::arg("path"), py::arg("default_kv"), py::arg("user_data")
    );

    m.def("fdb_tsdb_init",
        [](fdb_tsdb_t kvdb, const char *name, const char *path, size_t max_len) -> fdb_err_t
        {
            return fdb_tsdb_init(kvdb, name, path, get_time, max_len, NULL);
        },
        "Initialize TSDB",
        py::arg("db"),
        py::arg("name"),
        py::arg("path"),
        py::arg("max_len")
    );
    
    m.def("fdb_blob_make", &fdb_blob_make, "Create a blob object",
        py::arg("blob"), py::arg("value_buf"), py::arg("buf_len"));
    
    m.def("fdb_kv_get_blob", &fdb_kv_get_blob, "Get a blob from KVDB",
        py::arg("kvdb"), py::arg("key"), py::arg("blob"));
    
    m.def("fdb_kv_set_blob", &fdb_kv_set_blob, "Set a blob in KVDB",
        py::arg("kvdb"), py::arg("key"), py::arg("blob"));

    m.def("fdb_kv_set", &fdb_kv_set,
        "Set a string in KVDB",
        py::arg("kvdb"), py::arg("key"), py::arg("value"));

    /* fdb_kv_get returns char*, which will be directly convert to Python string by pybind11 */
    m.def("fdb_kv_get", &fdb_kv_get, "Get a string from KVDB",
        py::arg("kvdb"), py::arg("key"));

    m.def("set_env_status_kv",
        [](fdb_kvdb_t kvdb, const char *key, struct env_status *status) -> int {
            struct fdb_blob blob;
            return fdb_kv_set_blob(kvdb, key, fdb_blob_make(&blob, status, sizeof(*status)));
        },
        "Set environment status in KVDB",
        py::arg("kvdb"), py::arg("key"), py::arg("status"));

    m.def("get_env_status_kv",
        [](fdb_kvdb_t kvdb, const char *key, struct env_status *status) -> int {
            struct fdb_blob blob;
            return fdb_kv_get_blob(kvdb, key, fdb_blob_make(&blob, status, sizeof(*status)));
        },
        "Get environment status from KVDB",
        py::arg("kvdb"), py::arg("key"), py::arg("status"));
    
    m.def("set_env_status_ts",
        [](fdb_tsdb_t tsdb, struct env_status *status) -> int {
            struct fdb_blob blob;
            return fdb_tsl_append(tsdb, fdb_blob_make(&blob, status, sizeof(*status)));
        },
        "Set environment status in TSDB",
        py::arg("tsdb"), py::arg("status"));
    
    m.def("tsdb_env_status_iter",
        [](fdb_tsdb_t tsdb) {
            fdb_tsl_iter(tsdb, query_cb, tsdb);
        },
        "Iterate over all TSL in TSDB",
        py::arg("tsdb")
    );
    
    /* read last saved time for simulated timestamp */
    m.def("fdb_tsdb_get_last_time",
        [](fdb_tsdb_t tsdb){
            fdb_tsdb_control(tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);
        },
        "Update the static variable counts for simulated timestamp",
        py::arg("tsdb")
    );
}
