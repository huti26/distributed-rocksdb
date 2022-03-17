#ifndef MASTER_THESIS_DOCKER_DRDBSERVERAPP_H
#define MASTER_THESIS_DOCKER_DRDBSERVERAPP_H

#include <iostream>
#include "rocksdb/db.h"
#include "glog/logging.h"
#include "ucp/api/ucp.h"
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include "vector"
#include "DrdbWorker.h"
#include <thread>
#include "DrdbConnectionAcceptor.h"
#include <sys/epoll.h>
#include <fstream>
#include <mutex>
#include "../Client/DrdbClient.h"
#include "DrdbConnectionDeque.h"


class DrdbServerApp {

    int argc;
    char **argv;

    // Shared UCP Context
    ucp_context_h ucp_context{};

    std::map<int, std::shared_ptr<DrdbWorker>> drdb_workers;
    std::unordered_map<int, std::vector<std::shared_ptr<DrdbClient>>> redirect_clients;

    // Server Config
    int server_id{};
    int server_count{};
    std::string server_name{};
    uint16_t server_port{};
    std::string server_config_path{};
    std::string log_path = "/tmp/drdb/logs/server";
    std::string db_path = "/tmp/testdb";
    int redirect_clients_pro_server = 1;

    // RocksDB
    rocksdb::DB *db{};

    void connect_to_other_servers();

    void read_server_config();

    void init_ucp_context();

    void parse_cli();

public:
    DrdbServerApp(int argc, char **argv) {
        this->argc = argc;
        this->argv = argv;
    }

    int start();

    void init_rocksdb();
};


#endif //MASTER_THESIS_DOCKER_DRDBSERVERAPP_H
