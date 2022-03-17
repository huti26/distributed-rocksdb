#ifndef MASTER_THESIS_DOCKER_DRDBCLIENT_H
#define MASTER_THESIS_DOCKER_DRDBCLIENT_H

#include <random>
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
#include "mutex"


class DrdbClient {

    // UCP
    ucp_context_h ucp_context{};
    ucp_worker_h ucp_data_worker{};
    ucp_ep_h ucp_data_ep{};

    // Send
    int send_request_id = 0;
    ucp_request_context send_request_context{};
    ucp_request_context *send_request{};
    void *outBuffer = nullptr;
    ucp_request_param_t send_param{};

    // Receive
    int receive_request_id = 0;
    ucp_request_context receive_request_context{};
    ucp_request_context *receive_request{};
    void *inBuffer = nullptr;
    ucp_request_param_t receive_param{};
    size_t received_amount_of_bytes{};
    size_t expected_amount_of_bytes{};
    unsigned long value_length{};

    // Connection
    struct sockaddr_in *socket_address{};
    struct sockaddr_storage socket_address_storage{};
    uint16_t server_port;
    std::string server_address;

    ucp_request_param_t default_receive_request_param_t();

    void post_receive_cleanup();

    request_reply receive_status(Method method);

    request_reply receive_value_count();

    request_reply receive_value();

    ucp_request_param_t default_send_request_param_t();

    void post_send_cleanup();

    request_reply prepare_outBuffer_with_key(const std::string &key, Method method);

    void create_ep_and_ping();

    void init_ucp_context();

    void init_ucp_worker();

    static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status);

    static void send_cb(void *request, ucs_status_t status, void *user_data);

    static void receive_cb(void *request, ucs_status_t status, size_t length, void *user_data);

    void init_network();

    ucs_status_t progress_send_request();

    void benchmark(int benchmark_object_amount, int benchmark_object_size);

    std::vector<std::string> random_string_vector(int vector_length, unsigned long string_length);

    std::string random_string(unsigned long length);

    request_reply ping();

    ucs_status_t progress_receive_request();

    request_reply
    prepare_outBuffer_with_key_and_value(const std::string &key, void *send_value, unsigned long send_value_length);

    void benchmark_unknown_keys(int benchmark_object_amount, int benchmark_object_size);

    void benchmark_put(int benchmark_object_amount, int benchmark_object_size);

public:

    std::mutex mutex{};

    int id;

    DrdbClient(int id, const std::string &server_address, uint16_t server_port) {
        this->id = id;
        this->server_address = server_address;
        this->server_port = server_port;

        init_network();
        init_ucp_context();
        init_ucp_worker();
        create_ep_and_ping();

        LOG(INFO) << "Client-" << id << " Successfully connected to Server " << server_address << ":" << server_port;
    }

    DrdbClient(int id, const std::string &server_address, uint16_t server_port, ucp_context_h *ucp_context_p) {
        this->id = id;
        this->server_address = server_address;
        this->server_port = server_port;

        init_network();
        this->ucp_context = *ucp_context_p;
        init_ucp_worker();
        create_ep_and_ping();

        LOG(INFO) << "Client-" << id << " Successfully connected to Server " << server_address << ":" << server_port;
    }

    request_reply get(const std::string &key);

    request_reply del(const std::string &key);

    request_reply put(const std::string &key, void *send_value, unsigned long send_value_length);

    std::thread benchmark_thread(int benchmark_object_amount, int benchmark_object_size);

    std::thread benchmark_unknown_keys_thread(int benchmark_object_amount, int benchmark_object_size);

    request_reply exit();

    std::thread benchmark_put_thread(int benchmark_object_amount, int benchmark_object_size);
};


#endif //MASTER_THESIS_DOCKER_DRDBCLIENT_H
