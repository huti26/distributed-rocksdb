#include "DrdbConnectionAcceptor.h"

std::thread DrdbConnectionAcceptor::acceptorThread() {
    return std::thread(&DrdbConnectionAcceptor::runnable, this);
}


[[noreturn]] int DrdbConnectionAcceptor::runnable() {
    VLOG(INFO) << "Started Acceptor";

    VLOG(INFO) << "Initialising Client Connection Worker";
    init_acceptor_ucp_worker();

    VLOG(INFO) << "Setting up Client Connection Listener";
    setup_client_connection_listener();

    while (true) {
        while (connection_deque_p->connection_contexts.empty()) {
            ucp_worker_progress(acceptor_ucp_worker);
        }
    }
}

void DrdbConnectionAcceptor::setup_client_connection_listener() {
    // Setup Listener params
    ucp_listener_params_t params;
    params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                        UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;
    params.sockaddr.addr = (const struct sockaddr *) &socket_address_storage;
    params.sockaddr.addrlen = sizeof(socket_address_storage);
    params.conn_handler.cb = queue_connection_request;
    params.conn_handler.arg = connection_deque_p.get();

    /* Create a listener on the Worker side to listen on the given address.*/
    auto status = ucp_listener_create(
            acceptor_ucp_worker,
            &params,
            &listener
    );
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_listener_create " << ucs_status_string(status);
        std::exit(-1);
    }
}

void DrdbConnectionAcceptor::queue_connection_request(ucp_conn_request_h conn_request, void *arg) {
    VLOG(INFO) << "Adding Connection Request to Queue";

    auto *drdbConnectionDeque = static_cast<DrdbConnectionDeque *>(arg);
    drdbConnectionDeque->mutex.lock();
    drdbConnectionDeque->connection_contexts.push_back(conn_request);
    drdbConnectionDeque->mutex.unlock();

    VLOG(INFO) << "Successfully Addded Connection Request to Queue";
}


void DrdbConnectionAcceptor::init_acceptor_ucp_worker() {
    ucp_worker_params_t worker_params;
    ucs_status_t status;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
//    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    status = ucp_worker_create(ucp_context, &worker_params, &acceptor_ucp_worker);
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_worker_create " << ucs_status_string(status);
        std::exit(-1);
    }
}