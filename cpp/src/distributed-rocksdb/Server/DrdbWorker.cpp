#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

#include "DrdbWorker.h"

/*
 * THREADING
 */

std::thread DrdbWorker::runnableThread() {
    return std::thread(&DrdbWorker::work, this);
}

// Status: -1 error, 0 ok, 1 ucs_inprogress (should never happen in sync worker), 2 exit
void DrdbWorker::work() {
    int status = 0;
    while (status == 0) {
        status = receive_method();
    }
    VLOG(INFO) << "Worker-" << id << " FINISHED - STATUS " << status;
}

/*
 * RECEIVE
 */

int DrdbWorker::receive_method() {
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_METHOD";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_METHOD " << "Allocated inBuffer";
    inBuffer = malloc(sizeof(Method));
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = sizeof(Method);

    // Prepare Params & Receive Request
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_METHOD " << "created receive_request";
    receive_param = default_receive_request_param_t();

    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    sizeof(Method),
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request(
            receive_request,
            &receive_request_context,
            "RECEIVE_METHOD"
    );
    if (ucs_status == UCS_INPROGRESS) {
        return 1;
    }
    if (ucs_status != UCS_OK) {
        return -1;
    }

    auto received_method = static_cast<Method *>(inBuffer);

    switch (*received_method) {
        case PING:
            VLOG(INFO) << "Worker-" << id << " RECEIVE_METHOD PING";
            post_receive_cleanup();
            method = PING;
            return respond_ping();
        case GET:
            VLOG(INFO) << "Worker-" << id << " RECEIVE_METHOD GET";
            post_receive_cleanup();
            method = GET;
            return receive_key_count();
        case PUT:
            VLOG(INFO) << "Worker-" << id << " RECEIVE_METHOD PUT";
            post_receive_cleanup();
            method = PUT;
            return receive_key_count();
        case DEL:
            VLOG(INFO) << "Worker-" << id << " RECEIVE_METHOD DEL";
            post_receive_cleanup();
            method = DEL;
            return receive_key_count();
        case EXT:
            VLOG(INFO) << "Worker-" << id << " RECEIVE_METHOD EXT, closing Endpoint";
            ep_close();
            ucp_worker_destroy(ucp_data_worker);
            return 2;
        default:
            VLOG(ERROR) << "Worker-" << id << " RECEIVE_METHOD Unknown Method! \"" << *received_method << "\"";
            return -1;
    }

}

int DrdbWorker::receive_key_count() {
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY_COUNT";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY_COUNT " << "Allocated inBuffer";
    inBuffer = malloc(sizeof(unsigned long));
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = sizeof(unsigned long);

    // Prepare Params & Receive Request
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY_COUNT " << "created receive_request";
    receive_param = default_receive_request_param_t();

    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    sizeof(unsigned long),
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request(
            receive_request,
            &receive_request_context,
            "RECEIVE_KEY_COUNT"
    );
    if (ucs_status == UCS_INPROGRESS) {
        return 1;
    }
    if (ucs_status != UCS_OK) {
        return -1;
    }

    auto received_key_count = static_cast<unsigned long *>(inBuffer);
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY_COUNT \"" << *received_key_count << "\"";
    key_length = *received_key_count;
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY_COUNT Set key_length to " << key_length;

    // Cleanup
    post_receive_cleanup();

    return receive_key();
}

int DrdbWorker::receive_key() {
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY " << "Allocated inBuffer, length " << key_length;
    inBuffer = malloc(key_length);
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = key_length;

    // Prepare Params & Receive Request
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY " << "created receive_request";
    receive_param = default_receive_request_param_t();

    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    key_length,
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request(
            receive_request,
            &receive_request_context,
            "RECEIVE_KEY"
    );
    if (ucs_status == UCS_INPROGRESS) {
        return 1;
    }
    if (ucs_status != UCS_OK) {
        return -1;
    }

    auto received_key = static_cast<char *>(inBuffer);
    key = std::string(received_key, key_length);
    auto key_hash = hasher(key);

    VLOG(INFO) << "Worker-" << id << " RECEIVE_KEY \"" << key << "\"";
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY length \"" << key.length() << "\"";
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_KEY hash \"" << hasher(key) << "\"";

    responsible_server = key_hash % server_count;
    VLOG(INFO) << "Worker-" << id << " SERVER_ID \"" << server_id << "\"" <<
               " RESPONSIBLE SERVER \"" << responsible_server << "\"";
    if (responsible_server != server_id) {
        redirect = true;
    } else {
        redirect = false;
    }

    switch (method) {
        case GET:
            post_receive_cleanup();
            return respond_get();
        case PUT:
            post_receive_cleanup();
            return receive_value_count();
        case DEL:
            post_receive_cleanup();
            return respond_del();
        default:
            VLOG(ERROR) << "Worker-" << id << " RECEIVE_KEY This should never occur, shutting down...";
            std::terminate();
    }

}

int DrdbWorker::receive_value_count() {
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE_COUNT";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE_COUNT " << "Allocated inBuffer";
    inBuffer = malloc(sizeof(unsigned long));
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = sizeof(unsigned long);

    // Prepare Params & Receive Request
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE_COUNT " << "created receive_request";
    receive_param = default_receive_request_param_t();

    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    sizeof(unsigned long),
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request(
            receive_request,
            &receive_request_context,
            "RECEIVE_VALUE_COUNT"
    );
    if (ucs_status == UCS_INPROGRESS) {
        return 1;
    }
    if (ucs_status != UCS_OK) {
        return -1;
    }

    auto received_value_count = static_cast<unsigned long *>(inBuffer);
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE_COUNT \"" << *received_value_count << "\"";
    value_length = *received_value_count;
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE_COUNT Set value_length to " << value_length;

    // Cleanup
    post_receive_cleanup();

    return receive_value();
}

int DrdbWorker::receive_value() {
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE " << "Allocated inBuffer, length " << value_length;
    inBuffer = malloc(value_length);
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = value_length;

    // Prepare Params & Receive Request
    VLOG(VERBOSE) << "Worker-" << id << " RECEIVE_VALUE " << "created receive_request";
    receive_param = default_receive_request_param_t();

    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    value_length,
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request(
            receive_request,
            &receive_request_context,
            "RECEIVE_VALUE"
    );
    if (ucs_status == UCS_INPROGRESS) {
        return 1;
    }
    if (ucs_status != UCS_OK) {
        return -1;
    }

    auto received_value = static_cast<char *>(inBuffer);
    value = std::string(received_value, value_length);

    VLOG(INFO) << "Worker-" << id << " RECEIVE_VALUE \"" << value << "\"";

    // Cleanup
    post_receive_cleanup();

    return respond_put();
}

void DrdbWorker::post_receive_cleanup() {
    receive_request_id = receive_request_id + 1;
    receive_request_context.id = receive_request_id;
    receive_request_context.complete = 0;
    receive_request = nullptr;
    received_amount_of_bytes = 0;
    expected_amount_of_bytes = 0;

    free(inBuffer);
    inBuffer = nullptr;
}

ucp_request_param_t DrdbWorker::default_receive_request_param_t() {
    ucp_request_param_t ucpRequestParam{};
    ucpRequestParam.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                                   UCP_OP_ATTR_FIELD_DATATYPE |
                                   UCP_OP_ATTR_FIELD_USER_DATA;

    ucpRequestParam.cb.recv_stream = receive_cb;
    ucpRequestParam.datatype = ucp_dt_make_contig(1);
    ucpRequestParam.user_data = &receive_request_context;

    ucpRequestParam.op_attr_mask |= UCP_OP_ATTR_FIELD_FLAGS;
    ucpRequestParam.flags = UCP_STREAM_RECV_FLAG_WAITALL;

    return ucpRequestParam;
}

ucs_status_t DrdbWorker::progress_receive_request(ucp_request_context *request, ucp_request_context *context,
                                                  const std::string &current_method) {

    // Operation done immidiately
    // Due to WAIT_ALL Flag there should never be missing bytes
    if (request == nullptr) {
        VLOG(VERBOSE) << "Worker-" << id << " " << current_method << " Request completed immidiately";
        VLOG(VERBOSE) << "Worker-" << id << " " << current_method << " received_amount_of_bytes "
                      << received_amount_of_bytes;
        VLOG(VERBOSE) << "Worker-" << id << " " << current_method << " expected_amount_of_bytes "
                      << expected_amount_of_bytes;
        return UCS_OK;
    }

    // Operation failed
    if (UCS_PTR_IS_ERR(request)) {
        VLOG(ERROR) << "Worker-" << id << " " << current_method << " RECEIVE_REQUEST failed with: "
                    << UCS_PTR_IS_ERR(request) << " " << ucs_status_string(UCS_PTR_STATUS(request));
        return UCS_PTR_STATUS(request);
    }

    // Operation not done immidiately
    VLOG(VERBOSE) << "Worker-" << id << " " << current_method << " Waiting for request " << request->id << " to finish";
    while (context->complete == 0) {
        ucp_worker_progress(ucp_data_worker);
    }
    ucs_status_t status = ucp_request_check_status(request);

    ucp_request_free(request);
    *request = ucp_request_context{};

    return status;
}

/*
 * SEND
 */

int DrdbWorker::respond_ping() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_PING";
    result.drdbStatus = OK;
    return send_status();
}

int DrdbWorker::respond_get() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_GET";

    if (redirect) {
        return redirect_get();
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_GET ANSWERING SELF";
    auto rocks_status = db->Get(rocksdb::ReadOptions(), key, &result.value);

    if (rocks_status.ok()) {
        result.drdbStatus = OK;
        return send_status_and_value();
    }

    if (rocks_status.IsNotFound()) {
        result.drdbStatus = KEY_NOT_FOUND;
        return send_status();
    }

    result.drdbStatus = UNKNOWN_ERROR;
    return send_status();
}

int DrdbWorker::redirect_get() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_GET REDIRECTING";
    std::vector<std::shared_ptr<DrdbClient>> clients_to_responsible_server;

    // Find client_to_responsible_server in map
    clients_to_responsible_server = redirect_clients.at(responsible_server);

    // Iterate redirect_clients till one is avaible
    bool got_client = false;
    while (!got_client) {
        for (const auto &client_to_responsible_server: clients_to_responsible_server) {
            got_client = client_to_responsible_server->mutex.try_lock();
            if (got_client) {
                result = client_to_responsible_server->get(key);
                client_to_responsible_server->mutex.unlock();
                break;
            }
        }
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_GET REDIRECTING DONE";

    if (result.drdbStatus == OK) {
        return send_status_and_value();
    }
    return send_status();
}

int DrdbWorker::respond_put() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_PUT";

    if (redirect) {
        return redirect_put();
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_PUT ANSWERING SELF";
    auto rocks_status = db->Put(rocksdb::WriteOptions(), key, value);

    if (rocks_status.ok()) {
        result.drdbStatus = OK;
        return send_status();
    }

    result.drdbStatus = UNKNOWN_ERROR;
    return send_status();
}

int DrdbWorker::redirect_put() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_PUT REDIRECTING";
    std::vector<std::shared_ptr<DrdbClient>> clients_to_responsible_server;

    // Find client_to_responsible_server in map
    clients_to_responsible_server = redirect_clients.at(responsible_server);

    // Iterate redirect_clients till one is avaible
    bool got_client = false;
    while (!got_client) {
        for (const auto &client_to_responsible_server: clients_to_responsible_server) {
            got_client = client_to_responsible_server->mutex.try_lock();
            if (got_client) {
                result = client_to_responsible_server->put(key, value.data(), value.length());
                client_to_responsible_server->mutex.unlock();
                break;
            }
        }
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_PUT REDIRECTING DONE";
    return send_status();
}

int DrdbWorker::respond_del() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_DEL";

    if (redirect) {
        return redirect_del();
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_DEL ANSWERING SELF";
    auto rocks_status = db->Delete(rocksdb::WriteOptions(), key);

    if (rocks_status.ok()) {
        result.drdbStatus = OK;
        return send_status();
    }

    if (rocks_status.IsNotFound()) {
        result.drdbStatus = KEY_NOT_FOUND;
        return send_status();
    }

    result.drdbStatus = UNKNOWN_ERROR;
    return send_status();
}

int DrdbWorker::redirect_del() {
    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_DEL REDIRECTING";
    std::vector<std::shared_ptr<DrdbClient>> clients_to_responsible_server;

    // Find client_to_responsible_server in map
    clients_to_responsible_server = redirect_clients.at(responsible_server);

    // Iterate redirect_clients till one is avaible
    bool got_client = false;
    while (!got_client) {
        for (const auto &client_to_responsible_server: clients_to_responsible_server) {
            got_client = client_to_responsible_server->mutex.try_lock();
            if (got_client) {
                result = client_to_responsible_server->del(key);
                client_to_responsible_server->mutex.unlock();
                break;
            }
        }
    }

    VLOG(VERBOSE) << "Worker-" << id << " RESPOND_DEL REDIRECTING DONE";
    return send_status();
}

int DrdbWorker::send_status() {
    VLOG(VERBOSE) << "Worker-" << id << " SEND_STATUS";

    // Prepare Buffer
    VLOG(VERBOSE) << "Worker-" << id << " SEND_STATUS " << "Allocated inBuffer, length ";
    outBuffer = malloc(sizeof(DrdbStatus));
    memset(outBuffer, 0, sizeof(outBuffer));
    memcpy(outBuffer, &result.drdbStatus, sizeof(DrdbStatus));

    // Prepare Send Params & Request
    VLOG(VERBOSE) << "Worker-" << id << " SEND_STATUS " << "created send_request";
    send_param = default_send_request_param_t();

    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    sizeof(DrdbStatus),
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request(send_request, &send_request_context);
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Error at progress_send_request " << ucs_status_string(ucs_status);
        return -1;
    }

    VLOG(INFO) << "Worker-" << id << " Sent status " << DrdbStatusString(result.drdbStatus);
    post_send_cleanup();
    return 0;
}

int DrdbWorker::send_status_and_value() {
    VLOG(VERBOSE) << "Worker-" << id << " send_status_and_value " << "Allocated outBuffer";
    outBuffer = malloc(sizeof(DrdbStatus) + sizeof(unsigned long) + result.value.length());
    memset(outBuffer, 0, sizeof(outBuffer));

    // Copy Method
    memcpy(
            outBuffer,
            &result.drdbStatus,
            sizeof(DrdbStatus)
    );

    // Copy VALUE_COUNT
    auto result_value_length = result.value.length();
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(DrdbStatus),
            &result_value_length,
            sizeof(unsigned long)
    );

    // Copy VALUE
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(DrdbStatus) + sizeof(unsigned long),
            result.value.data(),
            result.value.length()
    );

    // Prepare Send Params & Request
    VLOG(VERBOSE) << "Worker-" << id << " send_status_and_value " << "created send_request";
    send_param = default_send_request_param_t();

    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    sizeof(DrdbStatus) + sizeof(unsigned long) + result.value.length(),
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request(send_request, &send_request_context);
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Worker-" << id << " Error at progress_send_request " << ucs_status_string(ucs_status);
        return -1;
    }

    VLOG(INFO) << "Worker-" << id << " Sent status " << DrdbStatusString(result.drdbStatus) << " and value "
               << result.value;
    post_send_cleanup();
    return 0;
}

void DrdbWorker::post_send_cleanup() {
    send_request_id = send_request_id + 1;
    send_request_context.id = send_request_id;
    send_request_context.complete = 0;
    send_request = nullptr;

    free(outBuffer);
    outBuffer = nullptr;
}

ucp_request_param_t DrdbWorker::default_send_request_param_t() {
    ucp_request_param_t ucpRequestParam{};
    ucpRequestParam.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                                   UCP_OP_ATTR_FIELD_DATATYPE |
                                   UCP_OP_ATTR_FIELD_USER_DATA;

    ucpRequestParam.datatype = ucp_dt_make_contig(1);
    ucpRequestParam.cb.send = send_cb;
    ucpRequestParam.user_data = &send_request_context;

    return ucpRequestParam;
}

ucs_status_t DrdbWorker::progress_send_request(ucp_request_context *request, ucp_request_context *context) {

    // Operation done immidiately
    if (request == nullptr) {
        VLOG(VERBOSE) << "Worker-" << id << " SEND_REQUEST completed immidiately";
        return UCS_OK;
    }

    // Operation failed
    if (UCS_PTR_IS_ERR(request)) {
        VLOG(ERROR) << "Worker-" << id << " SEND_REQUEST Request failed with: "
                    << UCS_PTR_IS_ERR(request) << " " << ucs_status_string(UCS_PTR_STATUS(request));
        return UCS_PTR_STATUS(request);
    }

    // Operation not done immidiately
    VLOG(VERBOSE) << "Worker-" << id << " Waiting for SEND_REQUEST " << request->id << " to finish";
    while (context->complete == 0) {
        ucp_worker_progress(ucp_data_worker);
    }
    ucs_status_t status = ucp_request_check_status(request);

    ucp_request_free(request);

    return status;
}

/*
 * CALLBACKS
 */

void DrdbWorker::send_cb(void *request, ucs_status_t status, void *user_data) {
    VLOG(VERBOSE) << "send_cb called";

    if (user_data == nullptr) {
        VLOG(ERROR) << "user_data in send_cb null";
        return;
    }

    auto *current_ctx = static_cast<ucp_request_context *>(user_data);
    VLOG(VERBOSE) << "SEND_CB completed send_request with id " << current_ctx->id;
    VLOG(VERBOSE) << "SEND_CB SEND_REQUEST_CONTEXT ADDRESS " << current_ctx;
    current_ctx->complete = 1;
}

void DrdbWorker::receive_cb(void *request, ucs_status_t status, size_t length,
                            void *user_data) {
    VLOG(VERBOSE) << "receive_cb called";

    if (user_data == nullptr) {
        VLOG(ERROR) << "user_data in receive_cb null";
        return;
    }

    auto *current_ctx = static_cast<ucp_request_context *>(user_data);
    VLOG(VERBOSE) << "RECEIVE_CB COMPLETED RECEIVE_REQUEST_CONTEXT id " << current_ctx->id;
    VLOG(VERBOSE) << "RECEIVE_CB RECEIVE_REQUEST_CONTEXT ADDRESS " << current_ctx;
    current_ctx->complete = 1;
}

/*
 *  EP_CLOSE
 */

void DrdbWorker::ep_close() {
    ucp_request_param_t param;
    ucs_status_t status;
    void *close_req;

    param.op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS;
    close_req = ucp_ep_close_nbx(ucp_data_ep, &param);
    if (UCS_PTR_IS_PTR(close_req)) {
        do {
            ucp_worker_progress(ucp_data_worker);
            status = ucp_request_check_status(close_req);
        } while (status == UCS_INPROGRESS);
        ucp_request_free(close_req);
    } else {
        status = UCS_PTR_STATUS(close_req);
    }

    if (status != UCS_OK) {
        VLOG(3) << "Failed to close ep " << ucs_status_string(status);
    }
}


#pragma clang diagnostic pop