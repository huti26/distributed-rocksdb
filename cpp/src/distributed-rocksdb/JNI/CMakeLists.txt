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

# JNI
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# Create Shared Library
set(CMAKE_BUILD_TYPE Release)
set(LIBRARY_NAME "jdrdb-client")

add_library(${LIBRARY_NAME} SHARED JDrdbClient.cpp ../Client/DrdbClient.cpp)

target_link_libraries(
        ${LIBRARY_NAME}
        -lpthread -ldl
        -lglog
        -lrocksdb
        -L/opt/ucx-${UCX_VERSION}/install-release/lib
        -lucp -lucs -luct
)


# Install library
install(TARGETS ${LIBRARY_NAME} DESTINATION lib/${LIBRARY_NAME})

# Install library headers
install(FILES JDrdbClient.h ../Client/DrdbClient.h DESTINATION include/${LIBRARY_NAME})
