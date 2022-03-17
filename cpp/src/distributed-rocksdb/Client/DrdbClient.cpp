#include "DrdbClient.h"

/*
 * UCP
 */

void DrdbClient::init_network() {
    // Setup default Socket
    memset(
            &socket_address_storage,
            0,
            sizeof(socket_address_storage)
    );

    this->socket_address = (struct sockaddr_in *) &socket_address_storage;
    inet_pton(AF_INET, server_address.data(), &socket_address->sin_addr);
    this->socket_address->sin_family = AF_INET;
    this->socket_address->sin_port = htons(server_port);
}

void DrdbClient::init_ucp_context() {
    /* UCP objects */
    ucp_params_t ucp_params;
    ucs_status_t status;

    memset(&ucp_params, 0, sizeof(ucp_params));

    /* UCP initialization */
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;

    ucp_params.features = UCP_FEATURE_STREAM;

    status = ucp_init(&ucp_params, nullptr, &this->ucp_context);
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_init " << ucs_status_string(status);
        std::exit(-1);
    }
}

void DrdbClient::init_ucp_worker() {
    ucp_worker_params_t worker_params;
    ucs_status_t status;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
//    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;
    worker_params.thread_mode = UCS_THREAD_MODE_SERIALIZED;

    status = ucp_worker_create(this->ucp_context, &worker_params, &ucp_data_worker);
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_worker_create " << ucs_status_string(status);
        std::exit(-1);
    }
}

void DrdbClient::create_ep_and_ping() {
    ucp_ep_params_t ep_params;
    ucs_status_t status;

    ep_params.field_mask = UCP_EP_PARAM_FIELD_FLAGS |
                           UCP_EP_PARAM_FIELD_SOCK_ADDR |
                           UCP_EP_PARAM_FIELD_ERR_HANDLER |
                           UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    ep_params.err_handler.cb = err_cb;
    ep_params.err_handler.arg = nullptr;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (struct sockaddr *) &this->socket_address_storage;
    ep_params.sockaddr.addrlen = sizeof(socket_address_storage);

    status = ucp_ep_create(
            ucp_data_worker,
            &ep_params,
            &ucp_data_ep
    );
    if (status != UCS_OK) {
        VLOG(ERROR) << "error at ucp_ep_create " << ucs_status_string(status);
        std::exit(-1);
    }

    int attempt = 0;
    int max_attempts = 100;
    auto result = ping();
    while (result.drdbStatus != OK && attempt < max_attempts) {
        VLOG(ERROR) << "Client-" << id << " " << server_address << ":" << server_port
                    << " PING failed, retrying " << max_attempts - attempt << " more times";
        attempt++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        status = ucp_ep_create(
                ucp_data_worker,
                &ep_params,
                &ucp_data_ep
        );
        if (status != UCS_OK) {
            VLOG(ERROR) << "error at ucp_ep_create " << ucs_status_string(status);
            std::exit(-1);
        }

        result = ping();
    }

    if (result.drdbStatus != OK) {
        VLOG(ERROR) << "Client-" << id << " Failed to conect to Server, shutting down..";
        std::terminate();
    }

}

/*
 * SEND
 */

request_reply DrdbClient::ping() {
    VLOG(VERBOSE) << "Client-" << id << " PING";
    auto method = PING;

    // Prepare Buffer
    VLOG(VERBOSE) << "Client-" << id << " PING Allocated outBuffer";
    outBuffer = malloc(sizeof(Method));
    memset(outBuffer, 0, sizeof(outBuffer));

    // Copy Method
    memcpy(
            outBuffer,
            &method,
            sizeof(Method)
    );

    // Prepare Send Params
    send_param = default_send_request_param_t();

    // Prepare Send Request
    VLOG(VERBOSE) << "Client-" << id << " PING created send_request";
    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    sizeof(Method),
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " Ping failed with " << ucs_status_string(ucs_status);
        post_send_cleanup();
        return request_reply{UNKNOWN_ERROR};
    }

    post_send_cleanup();
    return receive_status(method);

}

request_reply DrdbClient::exit() {
    VLOG(VERBOSE) << "Client-" << id << " EXIT";
    auto method = EXT;

    // Prepare Buffer
    VLOG(VERBOSE) << "Client-" << id << " EXIT Allocated outBuffer";
    outBuffer = malloc(sizeof(Method));
    memset(outBuffer, 0, sizeof(outBuffer));

    // Copy Method
    memcpy(
            outBuffer,
            &method,
            sizeof(Method)
    );

    // Prepare Send Params
    send_param = default_send_request_param_t();

    // Prepare Send Request
    VLOG(VERBOSE) << "Client-" << id << " EXIT created send_request";
    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    sizeof(Method),
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " Error at progress_send_request " << ucs_status_string(ucs_status);
        return request_reply{UNKNOWN_ERROR};
    }

    post_send_cleanup();
//    ep_close();
//    ucp_worker_destroy(ucp_data_worker);
    return request_reply{OK};
}

request_reply DrdbClient::get(const std::string &key) {
    return prepare_outBuffer_with_key(key, GET);
}

request_reply DrdbClient::del(const std::string &key) {
    return prepare_outBuffer_with_key(key, DEL);
}

request_reply DrdbClient::put(const std::string &key, void *send_value, unsigned long send_value_length) {
    return prepare_outBuffer_with_key_and_value(key, send_value, send_value_length);
}

request_reply DrdbClient::prepare_outBuffer_with_key(const std::string &key, Method method) {
    if (method == GET) {
        VLOG(VERBOSE) << "Client-" << id << " GET prepare_outBuffer_with_key";
    } else {
        VLOG(VERBOSE) << "Client-" << id << " DEL prepare_outBuffer_with_key";
    }

    // Prepare Buffer
    auto key_length = key.length();

    VLOG(VERBOSE) << "Client-" << id << " prepare_outBuffer_with_key " << "Allocated outBuffer";
    outBuffer = malloc(sizeof(Method) + sizeof(unsigned long) + key_length);
    memset(outBuffer, 0, sizeof(outBuffer));

    // Copy Method
    memcpy(
            outBuffer,
            &method,
            sizeof(Method)
    );

    // Copy KEY_COUNT
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method),
            &key_length,
            sizeof(unsigned long)
    );

    // Copy KEY
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method) + sizeof(unsigned long),
            key.data(),
            key_length
    );

    // Prepare Send Params
    send_param = default_send_request_param_t();

    // Prepare Send Request
    VLOG(VERBOSE) << "Client-" << id << " prepare_outBuffer_with_key " << "created send_request";
    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    sizeof(Method) + sizeof(unsigned long) + key_length,
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " Error at progress_send_request " << ucs_status_string(ucs_status);
        return request_reply{UNKNOWN_ERROR};
    }

    post_send_cleanup();
    return receive_status(method);
}

request_reply DrdbClient::prepare_outBuffer_with_key_and_value(const std::string &key, void *send_value,
                                                               unsigned long send_value_length) {
    VLOG(VERBOSE) << "Client-" << id << " PUT prepare_outBuffer_with_key_and_value";

    Method method = PUT;
    // Prepare Buffer
    auto key_length = key.length();

    VLOG(VERBOSE) << "Client-" << id << " prepare_outBuffer_with_key " << "Allocated outBuffer";
    auto outBuffer_length =
            sizeof(Method) + sizeof(unsigned long) + key_length + sizeof(unsigned long) + send_value_length;
    outBuffer = malloc(outBuffer_length);
    memset(outBuffer, 0, sizeof(outBuffer));

    // Copy Method
    memcpy(
            outBuffer,
            &method,
            sizeof(Method)
    );

    // Copy KEY_COUNT
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method),
            &key_length,
            sizeof(unsigned long)
    );

    // Copy KEY
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method) + sizeof(unsigned long),
            key.data(),
            key_length
    );

    // Copy VALUE_COUNT
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method) + sizeof(unsigned long) + key_length,
            &send_value_length,
            sizeof(unsigned long)
    );

    // Copy VALUE
    memcpy(
            static_cast<char *>(outBuffer) + sizeof(Method) + sizeof(unsigned long) + key_length +
            sizeof(unsigned long),
            send_value,
            send_value_length
    );

    // Prepare Send Params
    send_param = default_send_request_param_t();

    // Prepare Send Request
    VLOG(VERBOSE) << "Client-" << id << " prepare_outBuffer_with_key " << "created send_request";
    send_request = static_cast<ucp_request_context *>(
            ucp_stream_send_nbx(
                    ucp_data_ep,
                    outBuffer,
                    outBuffer_length,
                    &send_param
            )
    );

    // Progress Send Request
    auto ucs_status = progress_send_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " Error at progress_send_request " << ucs_status_string(ucs_status);
        return request_reply{UNKNOWN_ERROR};
    }

    post_send_cleanup();
    return receive_status(method);
}

ucs_status_t DrdbClient::progress_send_request() {

    // Operation done immidiately
    if (send_request == nullptr) {
        VLOG(VERBOSE) << "Client-" << id << " SEND_REQUEST completed immidiately";
        return UCS_OK;
    }

    // Operation failed
    if (UCS_PTR_IS_ERR(send_request)) {
        VLOG(ERROR) << "Client-" << id << " SEND_REQUEST Request failed with: " << UCS_PTR_STATUS(send_request) << " "
                    << ucs_status_string(UCS_PTR_STATUS(send_request));
        return UCS_PTR_STATUS(send_request);
    }

    // Operation not done immidiately
    VLOG(VERBOSE) << "Client-" << id << " Waiting for SEND_REQUEST_CONTEXT id " << send_request_context.id;
    VLOG(VERBOSE) << "Client-" << id << " SEND_REQUEST_CONTEXT ADDRESS " << &send_request_context;
    while (send_request_context.complete == 0) {
        ucp_worker_progress(ucp_data_worker);
    }
    ucs_status_t status = ucp_request_check_status(send_request);

    ucp_request_free(send_request);

    return status;
}

void DrdbClient::post_send_cleanup() {
    send_request_id = send_request_id + 1;
    send_request_context.id = send_request_id;
    send_request_context.complete = 0;
    send_request = nullptr;

    free(outBuffer);
    outBuffer = nullptr;
}

ucp_request_param_t DrdbClient::default_send_request_param_t() {
    ucp_request_param_t ucpRequestParam{};
    ucpRequestParam.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                                   UCP_OP_ATTR_FIELD_DATATYPE |
                                   UCP_OP_ATTR_FIELD_USER_DATA;

    ucpRequestParam.datatype = ucp_dt_make_contig(1);
    ucpRequestParam.cb.send = send_cb;
    ucpRequestParam.user_data = &send_request_context;

    return ucpRequestParam;
}

/*
 * RECEIVE
 */

request_reply DrdbClient::receive_status(Method method) {
    VLOG(VERBOSE) << "Client-" << id << " Handling state RECEIVE_STATUS";

    // Prepare Buffer
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_STATUS " << "Allocated inBuffer";
    inBuffer = malloc(sizeof(DrdbStatus));
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = sizeof(DrdbStatus);

    // Prepare Params
    receive_param = default_receive_request_param_t();

    // Prepare Receive Request
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_STATUS " << "created receive_request";
    receive_request = static_cast<ucp_request_context *>(
            ucp_stream_recv_nbx(
                    ucp_data_ep,
                    inBuffer,
                    sizeof(DrdbStatus),
                    &received_amount_of_bytes,
                    &receive_param
            )
    );

    // Receive Stream
    auto ucs_status = progress_receive_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " progress_receive_request resulted in " << ucs_status_string(ucs_status);
        post_receive_cleanup();
        return request_reply{UNKNOWN_ERROR};
    }

    auto received_status = static_cast<DrdbStatus *>(inBuffer);
    VLOG(INFO) << "Client-" << id << " Received Status " << DrdbStatusString(*received_status);

    switch (*received_status) {
        case OK:
            if (method == GET) {
                post_receive_cleanup();
                return receive_value_count();
            }

            post_receive_cleanup();
            return request_reply{OK};

        case KEY_NOT_FOUND:
            post_receive_cleanup();
            return request_reply{KEY_NOT_FOUND};

        case UNKNOWN_ERROR:
            post_receive_cleanup();
            return request_reply{UNKNOWN_ERROR};

        default:
            VLOG(ERROR) << "Client-" << id << "Unkown Status " << DrdbStatusString(*received_status)
                        << " This should never occur, shutting down..";
            std::exit(-1);
    }

}

request_reply DrdbClient::receive_value_count() {
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE_COUNT";

    // Prepare Buffer
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE_COUNT " << "Allocated inBuffer";
    inBuffer = malloc(sizeof(unsigned long));
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = sizeof(unsigned long);

    // Prepare Params
    receive_param = default_receive_request_param_t();

    // Prepare Receive Request
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE_COUNT " << "created receive_request";
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
    auto ucs_status = progress_receive_request();
    if (ucs_status != UCS_OK) {
        return request_reply{UNKNOWN_ERROR};
    }

    auto received_value_count = static_cast<unsigned long *>(inBuffer);
    value_length = *received_value_count;
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE_COUNT \"" << *received_value_count << "\"";

    post_receive_cleanup();

    return receive_value();
}

request_reply DrdbClient::receive_value() {
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE";

    // Prepare Buffer
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE " << "Allocated inBuffer, length " << value_length;
    inBuffer = malloc(value_length);
    memset(inBuffer, 0, sizeof(inBuffer));
    expected_amount_of_bytes = value_length;

    // Prepare Params
    receive_param = default_receive_request_param_t();

    // Prepare Receive Request
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_VALUE " << "created receive_request";
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
    auto ucs_status = progress_receive_request();
    if (ucs_status != UCS_OK) {
        VLOG(ERROR) << "Client-" << id << " progress_receive_request resulted in " << ucs_status_string(ucs_status);
        return request_reply{UNKNOWN_ERROR};
    }

    auto received_value = static_cast<char *>(inBuffer);
    VLOG(INFO) << "Client-" << id << " RECEIVE_VALUE \"" << received_value << "\"";

    request_reply result{OK, std::string(received_value, value_length)};

    post_receive_cleanup();

    return result;
}

ucs_status_t DrdbClient::progress_receive_request() {

    // Operation done immidiately
    // No bytes should be missing due to WAIT_ALL flag
    if (receive_request == nullptr) {
        VLOG(VERBOSE) << "Client-" << id << " Request completed immidiately";
        VLOG(VERBOSE) << "Client-" << id << " bytes " << received_amount_of_bytes << "/" << expected_amount_of_bytes;

        return UCS_OK;
    }

    // Operation failed
    if (UCS_PTR_IS_ERR(receive_request)) {
        VLOG(ERROR) << "Client-" << id << " RECEIVE_REQUEST Request failed with: " << UCS_PTR_STATUS(receive_request)
                    << " " << ucs_status_string(UCS_PTR_STATUS(receive_request));
        return UCS_PTR_STATUS(receive_request);
    }

    // Operation not done immidiately
    VLOG(VERBOSE) << "Client-" << id << " Waiting for RECEIVE_REQUEST_CONTEXT id " << receive_request_context.id;
    VLOG(VERBOSE) << "Client-" << id << " RECEIVE_REQUEST_CONTEXT ADDRESS " << &receive_request_context;
    while (receive_request_context.complete == 0) {
        ucp_worker_progress(ucp_data_worker);
    }
    ucs_status_t status = ucp_request_check_status(receive_request);

    ucp_request_free(receive_request);
    *receive_request = ucp_request_context{};

    return status;
}

void DrdbClient::post_receive_cleanup() {
    receive_request_id = receive_request_id + 1;
    receive_request_context.id = receive_request_id;
    receive_request_context.complete = 0;
    receive_request = nullptr;
    received_amount_of_bytes = 0;
    expected_amount_of_bytes = 0;

    free(inBuffer);
    inBuffer = nullptr;
}

ucp_request_param_t DrdbClient::default_receive_request_param_t() {
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

/*
 * CALLBACKS
 */

void DrdbClient::send_cb(void *request, ucs_status_t status, void *user_data) {
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

void DrdbClient::err_cb(void *arg, ucp_ep_h ep, ucs_status_t status) {
    VLOG(ERROR) << "error at err_cb " << ucs_status_string(status);
}

void DrdbClient::receive_cb(void *request, ucs_status_t status, size_t length,
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
 * Benchmark
 */

std::thread DrdbClient::benchmark_thread(int benchmark_object_amount, int benchmark_object_size) {
    return std::thread(&DrdbClient::benchmark, this, benchmark_object_amount, benchmark_object_size);

}

std::thread DrdbClient::benchmark_unknown_keys_thread(int benchmark_object_amount, int benchmark_object_size) {
    return std::thread(&DrdbClient::benchmark_unknown_keys, this, benchmark_object_amount, benchmark_object_size);

}

std::thread DrdbClient::benchmark_put_thread(int benchmark_object_amount, int benchmark_object_size) {
    return std::thread(&DrdbClient::benchmark_put, this, benchmark_object_amount, benchmark_object_size);

}

void DrdbClient::benchmark(int benchmark_object_amount, int benchmark_object_size) {
    VLOG(ERROR) << "Client-" << id << " Preparing Benchmark";

    auto keys = random_string_vector(benchmark_object_amount, benchmark_object_size);
    auto values = random_string_vector(benchmark_object_amount, benchmark_object_size);
    std::vector<request_reply> results{};
    VLOG(ERROR) << "Client-" << id << " Starting Benchmark";

    auto start_time = std::chrono::high_resolution_clock::now();

    // PUT
    VLOG(INFO) << "Client-" << id << " PUT";
    for (int i = 0; i < benchmark_object_amount; i++) {
        VLOG(INFO) << "Client-" << id << " PUT " << i << " " << keys[i];
        results.push_back(put(keys[i], values[i].data(), values[i].length()));
    }

    // GET
    VLOG(INFO) << "Client-" << id << " GET";
    for (int i = 0; i < benchmark_object_amount; i++) {
        VLOG(INFO) << "Client-" << id << " GET " << i << " " << keys[i];
        results.push_back(get(keys[i]));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = duration_cast<std::chrono::milliseconds>(end_time - start_time);

    int ok_count = 0;
    int key_not_found_count = 0;
    int unknown_error_count = 0;

    for (auto &result: results) {
        switch (result.drdbStatus) {
            case OK:
                ok_count++;
                break;
            case KEY_NOT_FOUND:
                key_not_found_count++;
                break;
            case UNKNOWN_ERROR:
                unknown_error_count++;
                break;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    VLOG(ERROR) << "Client-" << id << " Finished Benchmark in " << duration.count() << "ms";
    VLOG(ERROR) << "Client-" << id << " Benchmark OK " << ok_count;
    VLOG(ERROR) << "Client-" << id << " Benchmark KEY_NOT_FOUND " << key_not_found_count;
    VLOG(ERROR) << "Client-" << id << " Benchmark UNKNOWN_ERROR " << unknown_error_count;

}

void DrdbClient::benchmark_put(int benchmark_object_amount, int benchmark_object_size) {
    VLOG(INFO) << "Client-" << id << " Preparing Benchmark";

    auto keys = random_string_vector(benchmark_object_amount, benchmark_object_size);
    auto values = random_string_vector(benchmark_object_amount, benchmark_object_size);
    std::vector<request_reply> results{};
    VLOG(INFO) << "Client-" << id << " Starting Benchmark";

    auto start_time = std::chrono::high_resolution_clock::now();

    // PUT
    VLOG(INFO) << "Client-" << id << " PUT";
    for (int i = 0; i < benchmark_object_amount; i++) {
        VLOG(INFO) << "Client-" << id << " PUT " << i << " " << keys[i];
        results.push_back(put(keys[i], values[i].data(), values[i].length()));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = duration_cast<std::chrono::milliseconds>(end_time - start_time);

    exit();

    int ok_count = 0;
    int key_not_found_count = 0;
    int unknown_error_count = 0;

    for (auto &result: results) {
        switch (result.drdbStatus) {
            case OK:
                ok_count++;
                break;
            case KEY_NOT_FOUND:
                key_not_found_count++;
                break;
            case UNKNOWN_ERROR:
                unknown_error_count++;
                break;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    VLOG(INFO) << "Client-" << id << " Finished Benchmark in " << duration.count() << "ms";
    VLOG(INFO) << "Client-" << id << " Benchmark OK " << ok_count;
    VLOG(INFO) << "Client-" << id << " Benchmark KEY_NOT_FOUND " << key_not_found_count;
    VLOG(INFO) << "Client-" << id << " Benchmark UNKNOWN_ERROR " << unknown_error_count;

}

void DrdbClient::benchmark_unknown_keys(int benchmark_object_amount, int benchmark_object_size) {
    VLOG(INFO) << "Client-" << id << " Preparing Benchmark";

    auto keys = random_string_vector(benchmark_object_amount, benchmark_object_size);
    auto values = random_string_vector(benchmark_object_amount, benchmark_object_size);
    std::vector<request_reply> results{};
    VLOG(INFO) << "Client-" << id << " Starting Benchmark";

    auto start_time = std::chrono::high_resolution_clock::now();

    // GET
    VLOG(INFO) << "Client-" << id << " GET";
    for (int i = 0; i < benchmark_object_amount; i++) {
        VLOG(INFO) << "Client-" << id << " GET " << i << " " << keys[i];
        results.push_back(get(keys[i]));
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = duration_cast<std::chrono::milliseconds>(end_time - start_time);

    int ok_count = 0;
    int key_not_found_count = 0;
    int unknown_error_count = 0;

    for (auto &result: results) {
        switch (result.drdbStatus) {
            case OK:
                ok_count++;
                break;
            case KEY_NOT_FOUND:
                key_not_found_count++;
                break;
            case UNKNOWN_ERROR:
                unknown_error_count++;
                break;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    VLOG(INFO) << "Client-" << id << " Finished Benchmark in " << duration.count() << "ms";
    VLOG(INFO) << "Client-" << id << " Benchmark OK " << ok_count;
    VLOG(INFO) << "Client-" << id << " Benchmark KEY_NOT_FOUND " << key_not_found_count;
    VLOG(INFO) << "Client-" << id << " Benchmark UNKNOWN_ERROR " << unknown_error_count;

}

std::string DrdbClient::random_string(std::string::size_type length) {
    static auto &chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{100};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(length);

    while (length--)
        s += chrs[pick(rg)];

    return s;
}

std::vector<std::string> DrdbClient::random_string_vector(int vector_length, std::string::size_type string_length) {
    std::vector<std::string> string_vector{};

    for (int i = 0; i < vector_length; i++) {
        string_vector.push_back(random_string(string_length));
    }

    return string_vector;
}

