import flashdb_test_lib

# KVDB operations
kvdb = flashdb_test_lib.fdb_kvdb()
flashdb_test_lib.fdb_kvdb_init(kvdb, "env", "fdb_kvdb1", None, None)

msg = "111111111111111"
flashdb_test_lib.fdb_kv_set(kvdb, "msg1", msg)
flashdb_test_lib.fdb_kv_get(kvdb, "msg1")

status = flashdb_test_lib.env_status()
status.temp = 20
status.humi = 60
flashdb_test_lib.set_env_status_kv(kvdb, "status", status)

saved_status = flashdb_test_lib.env_status()
saved_len = flashdb_test_lib.get_env_status_kv(kvdb, "status", saved_status)
print(f"{saved_len=}, {saved_status.temp=}, {saved_status.humi=}")


# TSDB operations
tsdb = flashdb_test_lib.fdb_tsdb()
flashdb_test_lib.fdb_tsdb_init(tsdb, "log", "fdb_tsdb1", 128)
flashdb_test_lib.fdb_tsdb_get_last_time(tsdb)

status.temp = 30
status.humi = 66
flashdb_test_lib.set_env_status_ts(tsdb, status)
flashdb_test_lib.tsdb_env_status_iter(tsdb)
for _ in range(10):
    status.temp += 1
    status.humi += 1
    flashdb_test_lib.set_env_status_ts(tsdb, status)