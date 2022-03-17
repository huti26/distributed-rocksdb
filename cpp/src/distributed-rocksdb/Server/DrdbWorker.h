#ifndef MASTER_THESIS_DOCKER_DRDBWORKER_H
#define MASTER_THESIS_DOCKER_DRDBWORKER_H

#include <iostream>
#include "rocksdb/db.h"
#include "glog/logging.h"
#include "ucp/api/ucp.h"
#include <arpa/inet.h>
#include "../Common/ClientServerCommons.h"
#include "../Client/DrdbClient.h"
#include <chrono>
#include <thread>
#include <sys/epoll.h>

class DrdbWorker {

    // UCP
    ucp_worker_h ucp_data_worker = nullptr;
    ucp_ep_h ucp_data_ep = nullptr;

    // UCP Receive
    int receive_request_id = 0;
    ucp_request_context receive_request_context{};
    ucp_request_context *receive_request{};
    void *inBuffer = nullptr;
    ucp_request_param_t receive_param{};

    // Redirect
    std::hash<std::string> hasher{};
    bool redirect = false;
    int server_id;
    int server_count;
    unsigned long responsible_server{};
    std::unordered_map<int, std::vector<std::shared_ptr<DrdbClient>>> redirect_clients;

    // UCP Send
    int send_request_id = 0;
    ucp_request_context send_request_context{};
    ucp_request_context *send_request{};
    void *outBuffer = nullptr;
    ucp_request_param_t send_param{};

    // Stores data received from Client
    Method method{};
    unsigned long key_length{};
    std::string key{};
    unsigned long value_length{};
    std::string value{};

    // Stores data to reply with to Client
    request_reply result{};

    // RocksDB
    rocksdb::DB *db;

    int receive_method();

    int receive_key_count();

    int receive_key();

    int receive_value_count();

    int receive_value();

    static void receive_cb(void *request, ucs_status_t status, size_t length, void *user_data);

    static void send_cb(void *request, ucs_status_t status, void *user_data);

    ucs_status_t progress_receive_request(ucp_request_context *request, ucp_request_context *context,
                                          const std::string &current_method);

    int respond_get();

    int respond_put();

    int respond_del();

    ucp_request_param_t default_receive_request_param_t();

    void post_receive_cleanup();

    int send_status();

    int send_status_and_value();

    ucp_request_param_t default_send_request_param_t();

    ucs_status_t progress_send_request(ucp_request_context *request, ucp_request_context *context);

    void post_send_cleanup();

    int respond_ping();

public:
    int id{};

    // length does not have to be in bytes when working with ucp
    // in this case we always use bytes
    size_t received_amount_of_bytes{};
    size_t expected_amount_of_bytes{};

    DrdbWorker(
            const std::shared_ptr<ucp_worker_h> &ucp_data_worker_p,
            const std::shared_ptr<ucp_ep_h> &ucp_data_ep_p,
            int id,
            int server_id, int server_count,
            std::unordered_map<int, std::vector<std::shared_ptr<DrdbClient>>> *redirect_clients_p,
            rocksdb::DB **db_p
    ) {
        this->ucp_data_worker = *ucp_data_worker_p;
        this->ucp_data_ep = *ucp_data_ep_p;
        this->id = id;
        this->server_id = server_id;
        this->server_count = server_count;
        this->redirect_clients = *redirect_clients_p;
        this->db = *db_p;

    }

    std::thread runnableThread();

    int redirect_get();

    int redirect_put();

    int redirect_del();

    void ep_close();

    void work();
};


#endif //MASTER_THESIS_DOCKER_DRDBWORKER_H
