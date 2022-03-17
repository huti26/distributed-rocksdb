#include "DrdbConnectionHandler.h"
#include "DrdbConnectionAcceptor.h"

std::thread DrdbConnectionHandler::handlerThread() {
    return std::thread(&DrdbConnectionHandler::handleConnections, this);
}

[[noreturn]] void DrdbConnectionHandler::handleConnections() {
    int current_id = 0;
    while (true) {
        VLOG(INFO) << "Waiting for connection...";
        while (connection_deque_p->connection_contexts.empty()) {}

        VLOG(INFO) << "Initialising new data worker";
        init_ucp_worker(data_ucp_workers.back());

        VLOG(INFO) << "Connection received, creating endpoint";
        create_ep();
        VLOG(INFO) << "Endpoint created";

        // Create new Worker which handles Client
        drdb_workers[current_id] = std::make_shared<DrdbWorker>(
                data_ucp_workers.back(),
                data_ucp_eps.back(),
                current_id,
                server_id,
                server_count,
                redirect_clients_p,
                &db
        );

        // Prepare for next Connection
        current_id = current_id + 1;
        data_ucp_workers.push_back(std::make_shared<ucp_worker_h>());
        data_ucp_eps.push_back(std::make_shared<ucp_ep_h>());
        connection_deque_p->mutex.lock();
        connection_deque_p->connection_contexts.pop_front();
        connection_deque_p->mutex.unlock();

        // Start the Worker
        std::thread worker_thread = drdb_workers[current_id - 1]->runnableThread();
        worker_thread.detach();

    }
}

void DrdbConnectionHandler::init_ucp_worker(const std::shared_ptr<ucp_worker_h> &ucp_worker_p) {
    ucp_worker_params_t worker_params;
    ucs_status_t status;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    status = ucp_worker_create(ucp_context, &worker_params, ucp_worker_p.get());
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_worker_create " << ucs_status_string(status);
        std::exit(-1);
    }
}

void DrdbConnectionHandler::create_ep() {
    ucp_ep_params_t ep_params;
    ucs_status_t ucsStatus;

    ep_params.field_mask = UCP_EP_PARAM_FIELD_ERR_HANDLER |
                           UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = connection_deque_p->connection_contexts.front();
    ep_params.err_handler.cb = err_cb;
    ep_params.err_handler.arg = nullptr;

    ucsStatus = ucp_ep_create(*data_ucp_workers.back().get(), &ep_params, data_ucp_eps.back().get());
    if (ucsStatus != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_ep_create " << ucs_status_string(ucsStatus);
        std::exit(-1);
    }
}


void DrdbConnectionHandler::err_cb(void *arg, ucp_ep_h ep, ucs_status_t status) {
    VLOG(ERROR) << "Error callback called with status " << status;
    VLOG(ERROR) << ucs_status_string(status);
}
