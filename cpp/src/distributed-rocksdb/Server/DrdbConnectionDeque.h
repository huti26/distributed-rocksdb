#ifndef MASTER_THESIS_DOCKER_DRDBCONNECTIONDEQUE_H
#define MASTER_THESIS_DOCKER_DRDBCONNECTIONDEQUE_H


#include <mutex>
#include <deque>
#include <ucp/api/ucp_def.h>

class DrdbConnectionDeque {
public:
    std::mutex mutex{};
    std::deque<ucp_conn_request_h> connection_contexts{};

};


#endif //MASTER_THESIS_DOCKER_DRDBCONNECTIONDEQUE_H
