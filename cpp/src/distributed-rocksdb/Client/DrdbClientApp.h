#ifndef MASTER_THESIS_DOCKER_DRDBCLIENTAPP_H
#define MASTER_THESIS_DOCKER_DRDBCLIENTAPP_H

#include <iostream>
#include <cstdint>
#include <cstring>
#include "rocksdb/db.h"
#include "glog/logging.h"
#include "ucp/api/ucp.h"
#include <arpa/inet.h>
#include "../Common/ClientServerCommons.h"
#include <chrono>
#include <thread>
#include "DrdbClient.h"


class DrdbClientApp {

    int argc;
    char **argv;

    std::string server_name{};
    uint16_t server_port = 0;
    bool start_interactive_cli = false;
    bool start_endless_loop = false;
    int benchmark_object_count = 1000;
    int benchmark_object_size = 5;

    void interactive_cli();

    void parse_cli();

    void benchmark_singlethreaded();

    void benchmark_multithreaded();

    void test_thread_creation();

public:
    DrdbClientApp(int argc, char **argv) {
        this->argc = argc;
        this->argv = argv;
    }

    int start();

    void benchmark_singlethreaded_unknown_keys();

    [[noreturn]] void benchmark_singlethreaded_endless();
};


#endif //MASTER_THESIS_DOCKER_DRDBCLIENTAPP_H
