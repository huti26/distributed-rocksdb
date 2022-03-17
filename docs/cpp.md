# cpp

Runconfigs for development with CLion can be found in `cpp/clion-run-configs`

The Client has a wrapper `DrdbClientApp` for development.

## CLI Args

### Server

```
-s SERVER_NAME -p PORT -c PATH_TO_CONFIG -d DATABASE_PATH -l LOGS_PATH -r REDIRECT_CLIENTS_PRO_SERVER
```

When starting via Clion Docker (without compose)

```
-s server1 -p 13337 -c /tmp/cpp/src/distributed-rocksdb/Server/server_config2.txt -d /tmp/testdb -l /tmp/drdb/logs

-s server2 -p 13338 -c /tmp/cpp/src/distributed-rocksdb/Server/server_config2.txt -d /tmp/testdb -l /tmp/drdb/logs
```

### Client

```
-s SERVER_NAME -p PORT -c BENCHMARK_OBJECTS_COUNT -l BENCHMARK_OBJECTS_LENGTH
```

```
-s server1 -p 13337
```

`-x` to enable interactive CLI

`-y` to start infinite benchmark loop

## Logging

7 -> Verbose

5 -> Info

3 -> Error
