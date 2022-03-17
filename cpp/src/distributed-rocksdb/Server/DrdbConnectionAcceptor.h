#ifndef MASTER_THESIS_DOCKER_DRDBCONNECTIONACCEPTOR_H
#define MASTER_THESIS_DOCKER_DRDBCONNECTIONACCEPTOR_H

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


class DrdbConnectionAcceptor {

    // UCP
    ucp_context_h ucp_context;
    std::shared_ptr<DrdbConnectionDeque> connection_deque_p;

    // Acceptor
    ucp_worker_h acceptor_ucp_worker{};
    ucp_listener_h listener{};

    // UCP Socket
    struct sockaddr_in *socket_address{};
    struct sockaddr_storage socket_address_storage{};
    uint16_t listener_port;

    void setup_client_connection_listener();

    static void queue_connection_request(ucp_conn_request_h conn_request, void *arg);

    [[noreturn]] int runnable();

    void init_acceptor_ucp_worker();

public:

    DrdbConnectionAcceptor(
            uint16_t listener_port, ucp_context_h *ucp_context_p,
            const std::shared_ptr<DrdbConnectionDeque>& connection_deque_p
    ) {

        this->ucp_context = *ucp_context_p;
        this->listener_port = listener_port;

        // Setup default Socket
        memset(
                &this->socket_address_storage,
                0,
                sizeof(this->socket_address_storage)
        );

        this->socket_address = (struct sockaddr_in *) &this->socket_address_storage;
        this->socket_address->sin_addr.s_addr = INADDR_ANY;
        this->socket_address->sin_family = AF_INET;
        this->socket_address->sin_port = htons(this->listener_port);

        this->connection_deque_p = connection_deque_p;
    }

    std::thread acceptorThread();

};


#endif //MASTER_THESIS_DOCKER_DRDBCONNECTIONACCEPTOR_H
