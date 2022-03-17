#ifndef MASTER_THESIS_DOCKER_CLIENTSERVERCOMMONS_H
#define MASTER_THESIS_DOCKER_CLIENTSERVERCOMMONS_H

// DRDB Methods
enum Method : uint8_t {
    GET, PUT, DEL, EXT, INVALID, PING
};

inline Method resolveMethod(const std::string &input) {
    if (input == "get") return GET;
    if (input == "put") return PUT;
    if (input == "del") return DEL;
    if (input == "ext") return EXT;
    return INVALID;
}

// DRDB Status
enum DrdbStatus : uint8_t {
    OK, KEY_NOT_FOUND, UNKNOWN_ERROR
};

inline std::string DrdbStatusString(DrdbStatus drdbStatus) {
    switch (drdbStatus) {
        case OK:
            return "OK";
        case KEY_NOT_FOUND:
            return "KEY_NOT_FOUND";
        case UNKNOWN_ERROR:
            return "UNKNOWN_ERROR";
    }

    return "DEFAULT";
}

// DRDB Result
struct request_reply {
    DrdbStatus drdbStatus;
    std::string value;
};

// DRDB Logging
enum VLogLevel : uint8_t {
    ERROR = 3, INFO = 5, VERBOSE = 7
};

// UCP Send/Receive Request Context
struct ucp_request_context {
    int complete = 0;
    int id = 0;
};

// Split std::string to vector
inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

#endif //MASTER_THESIS_DOCKER_CLIENTSERVERCOMMONS_H
