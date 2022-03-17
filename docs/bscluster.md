# bscluster

## Singularity

Request Resource

```sh
qsub -I -l ncpus=8,mem=15gb,nic_type=ib56,walltime=08:00:00
```

Download Image

```sh
singularity pull --arch amd64 library://hutan/drdb/drdb:latest
```

Create Config

```sh
mkdir configs
touch server1.txt
nano server1.txt
```

Run Container

```sh
singularity shell -B /home/moghaddam/ -B /media/ssd drdb_latest.sif
```

### Server

Start Server

```sh
/app/build/src/distributed-rocksdb/Server/distributed-rocksdb-server-cpp -s node68 -p 13337 -c /home/moghaddam/configs/server1.txt -l /media/ssd/drdb/logs/server -d /media/ssd/drdb/db -r 1
```

### Clients

#### YCSB

Start YCSB Manually

```sh
/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb load drdb -P /ycsb/drdb/workloads/workloada -p drdb.server=10.112.51.173 -p drdb.port=13337

/ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT/bin/ycsb run drdb -P /ycsb/drdb/workloads/workloada -p drdb.server=10.112.51.173 -p drdb.port=13337
```

Start multiple YCSBs with script in `/benchmarks`

```sh
time python3 client_load.py

time python3 client_run.py
```

#### CPP

Start Cpp-Client

```sh
/app/build/src/distributed-rocksdb/Client/distributed-rocksdb-client-cpp -s 10.112.51.173 -p 13337
```
