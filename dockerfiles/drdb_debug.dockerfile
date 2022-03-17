ARG UCX_VERSION=1.12.0

FROM hutii/drdb:base AS builder

WORKDIR /opt

# Needed for running YCSB
RUN apt install python2 python3 python-is-python2 -y

# YCSB
WORKDIR /ycsb

COPY ./ycsb .

RUN mvn -pl site.ycsb:drdb-binding -am clean package -Dcheckstyle.skip

WORKDIR /ycsb/drdb

RUN tar -xzf /ycsb/drdb/target/ycsb-drdb-binding-0.18.0-SNAPSHOT.tar.gz \
    && rm -rf /ycsb/drdb/target/ycsb-drdb-binding-0.18.0-SNAPSHOT.tar.gz

# DRDB JAVA
WORKDIR /java-app

COPY ./java/ .

# DRDB CPP
WORKDIR /app

COPY ./cpp/ .

# Build Everything - Client/Server & JNI-Library
WORKDIR /app/build
RUN cmake /app
RUN cmake --build .

RUN make
RUN make install

###################
# Set ENV variables
###################

# Surpress UCX Warning
ENV UCX_WARN_UNUSED_ENV_VARS=n

# Surpess Error Signals which clash with JVM
ENV UCX_ERROR_SIGNALS=""

# Export UCX Path
ENV UCX_PATH=/opt/ucx-$UCX_VERSION/install-release
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ucx-$UCX_VERSION/install-release/lib

# Add JNI-Library Path
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/jdrdb-client
RUN ldconfig /usr/local/lib/jdrdb-client/


###################
# RELEASE IMAGE
###################
FROM debian:11

# Install RocksDB
RUN apt update
RUN apt install librocksdb-dev -y

# Install glog
RUN apt install libgoogle-glog-dev -y

# Install Java
RUN apt-get install default-jre -y

# Later needed for running YCSB
RUN apt install python2 python3 python-is-python2 -y

###################
# Copy All binaries
###################

# UCX
ARG UCX_VERSION
COPY --from=builder /opt/ucx-$UCX_VERSION/install-release /opt/ucx-$UCX_VERSION/install-release

# Drdb-Cpp App
COPY --from=builder /app/build /app/build

# Drdb-Cpp App Server-configs
COPY ./cpp/src/distributed-rocksdb/Server/server_config1.txt /app/src/distributed-rocksdb/Server/server_config1.txt
COPY ./cpp/src/distributed-rocksdb/Server/server_config2.txt /app/src/distributed-rocksdb/Server/server_config2.txt
COPY ./cpp/src/distributed-rocksdb/Server/server_config4.txt /app/src/distributed-rocksdb/Server/server_config4.txt

# Drdb-Shared-Library
COPY --from=builder /usr/local/lib/jdrdb-client /usr/local/lib/jdrdb-client
COPY --from=builder /usr/local/include/jdrdb-client /usr/local/include/jdrdb-client

# YCSB
COPY --from=builder /ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT /ycsb/drdb/ycsb-drdb-binding-0.18.0-SNAPSHOT
COPY /ycsb/drdb/workloads /ycsb/drdb/workloads

# DRDB JAVA
COPY ./java/ ./java-app

###################
# Set ENV variables
###################

# Surpress UCX Warning
ENV UCX_WARN_UNUSED_ENV_VARS=n

# Surpess Error Signals which clash with JVM
ENV UCX_ERROR_SIGNALS=""

# Export UCX Path
ENV UCX_PATH=/opt/ucx-$UCX_VERSION/install-release
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/ucx-$UCX_VERSION/install-release/lib

# Add JNI-Library Path
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/jdrdb-client
RUN ldconfig /usr/local/lib/jdrdb-client/

