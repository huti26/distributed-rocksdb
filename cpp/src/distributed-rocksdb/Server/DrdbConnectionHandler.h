#ifndef MASTER_THESIS_DOCKER_DRDBCONNECTIONHANDLER_H
#define MASTER_THESIS_DOCKER_DRDBCONNECTIONHANDLER_H

#include <iostream>
#include "rocksdb/db.h"
#include "glog/logging.h"
#include "ucp/api/ucp.h"
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include "vector"
#include "DrdbWorker.h"
#include "../Client/DrdbClient.h"
#include "DrdbConnectionDeque.h"
#include <thread>
#include <deque>

class DrdbConnectionHandler {


    // UCP
    ucp_context_h ucp_context;
    std::shared_ptr<DrdbConnectionDeque> connection_deque_p;

    // DrdbWorkers
    std::map<int, std::shared_ptr<DrdbWorker>> drdb_workers;
    std::vector<std::shared_ptr<ucp_worker_h>> data_ucp_workers{};
    std::vector<std::shared_ptr<ucp_ep_h>> data_ucp_eps{};
    std::unordered_map<int, std::vector<std::shared_ptr<DrdbClient>>> *redirect_clients_p;

    // Server level info
    int server_id;
    int server_count;

    // RocksDB
    rocksdb::DB *db;

    [[noreturn]] void handleConnections();

    void init_ucp_worker(const std::shared_ptr<ucp_worker_h> &ucp_worker_p);

    void create_ep();

    static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status);

public:

    DrdbConnectionHandler(
            std::map<int, std::shared_ptr<DrdbWorker>> *drdb_workers_p,
            int server_id, int server_count,
            std::unordered_map<int, std::vector<std::shared_ptr<DrdbClient>>> *redirect_clients_p,
            ucp_context_h *ucp_context_p, rocksdb::DB **db_p,
            const std::shared_ptr<DrdbConnectionDeque> &connection_deque_p
    ) {
        this->ucp_context = *ucp_context_p;

        // Init Connection Objects
        data_ucp_workers.push_back(std::make_shared<ucp_worker_h>());
        data_ucp_eps.push_back(std::make_shared<ucp_ep_h>());
        this->drdb_workers = *drdb_workers_p;
        this->redirect_clients_p = redirect_clients_p;

        this->server_id = server_id;
        this->server_count = server_count;

        this->db = *db_p;

        this->connection_deque_p = connection_deque_p;
    }

    std::thread handlerThread();

};


#endif //MASTER_THESIS_DOCKER_DRDBCONNECTIONHANDLER_H
