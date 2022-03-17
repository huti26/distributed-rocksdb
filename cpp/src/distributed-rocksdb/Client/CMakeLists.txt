# Glog
find_library(GLOG glog required)
include_directories(/usr/include/glog)

# RocksDB
find_library(ROCKSDB rocksdb required)
include_directories(/usr/include/rocksdb)

# UCX
include_directories(
        "/opt/ucx-${UCX_VERSION}/install-release/include/"
)

# Create Executeable
add_executable(distributed-rocksdb-client-cpp main.cpp DrdbClient.cpp DrdbClient.h DrdbClientApp.cpp DrdbClientApp.h)

target_link_libraries(
        distributed-rocksdb-client-cpp
        -lpthread -ldl
        -lglog
        -lrocksdb
        -L/opt/ucx-${UCX_VERSION}/install-release/lib
        -lucp -lucs -luct
)
