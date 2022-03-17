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

add_executable(
        distributed-rocksdb-server-cpp
        main.cpp DrdbWorker.cpp DrdbWorker.h DrdbConnectionAcceptor.cpp DrdbConnectionAcceptor.h DrdbServerApp.cpp DrdbServerApp.h
        ../Client/DrdbClient.cpp ../Client/DrdbClient.h
        ../Client/DrdbClientApp.cpp ../Client/DrdbClientApp.h DrdbConnectionHandler.cpp DrdbConnectionHandler.h DrdbConnectionDeque.cpp DrdbConnectionDeque.h)

target_link_libraries(
        distributed-rocksdb-server-cpp
        -lpthread -ldl
        -lglog
        -lrocksdb
        -L/opt/ucx-${UCX_VERSION}/install-release/lib
        -lucp -lucs -luct
)
