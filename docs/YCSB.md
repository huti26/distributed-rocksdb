# YCSB

## Run with Docker

If you are using either hutii/drdb:debug or hutii/drdb:latest

### Start The Server

1 Server setup

```sh
/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server1 -p 13337 -c /app/src/distributed-rocksdb/Server/server_config1.txt
```

2 Server setup

```sh
/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server1 -p 13337 -c /app/src/distributed-rocksdb/Server/server_config2.txt

/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server2 -p 13338 -c /app/src/distributed-rocksdb/Server/server_config2.txt
```

4 Server setup

```sh
/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server1 -p 13337 -c /app/src/distributed-rocksdb/Server/server_config4.txt

/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server2 -p 13338 -c /app/src/distributed-rocksdb/Server/server_config4.txt

/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server3 -p 13339 -c /app/src/distributed-rocksdb/Server/server_config4.txt

/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s server4 -p 13340 -c /app/src/distributed-rocksdb/Server/server_config4.txt
```

### Start YCSB

Start YCSB CLI

```sh
/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb shell drdb -p drdb.server=server1 -p drdb.port=13337 -p drdb.log_dir=/tmp/test
```

Start Real Workload

```sh
/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb load drdb -P /ycsb/drdb/workloads/workloada -p drdb.server=server1 -p drdb.port=13337

/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb run drdb -P /ycsb/drdb/workloads/workloada -p drdb.server=server1 -p drdb.port=13337
```

## Notes

- `var` Keyword does not work
- /ycsb/bin/bindings.properties contains list of avaible databases
- /ycsb/bin/ycsb python file contains list of avaible database
- pom.xml in /ycsb contains list of databases to compile
- pom.xml in /ycsb/drdb contains compilation info and references to external jars
- mvn package creates tar.gz which includes another ycsb binary, this is the one to use -> not actually a binary, its a python script that calls java jar
- without empty README in drdb folder maven does not create a tar.gz
