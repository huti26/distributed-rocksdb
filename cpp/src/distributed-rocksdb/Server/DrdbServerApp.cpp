#include "DrdbServerApp.h"
#include "DrdbConnectionHandler.h"
#include <filesystem>


int DrdbServerApp::start() {
    FLAGS_logtostderr = true;
    FLAGS_v = 5;
    VLOG(INFO) << "Starting Server App.";

    parse_cli();
    init_rocksdb();
    init_ucp_context();
    read_server_config();

    // Connection Handling
    auto drdbConnectionDeque = std::make_shared<DrdbConnectionDeque>();

    DrdbConnectionAcceptor drdbConnectionAcceptor{
            server_port, &ucp_context, drdbConnectionDeque
    };

    DrdbConnectionHandler drdbConnectionHandler{
            &drdb_workers,
            server_id, server_count,
            &redirect_clients,
            &ucp_context, &db,
            drdbConnectionDeque
    };

    std::thread acceptorThread = drdbConnectionAcceptor.acceptorThread();
    std::thread handlerThread = drdbConnectionHandler.handlerThread();

    connect_to_other_servers();

    VLOG(INFO) << "Server is ready.";

    // log dir may only be set before init
    FLAGS_logtostderr = false;
    std::filesystem::create_directories(log_path);
    FLAGS_log_dir = log_path;
    google::InitGoogleLogging(argv[0]);
    FLAGS_minloglevel = 0;
    FLAGS_v = 3;

    VLOG(INFO) << "Waiting for Acceptor to finish..";
    acceptorThread.join();
    handlerThread.join();
    VLOG(INFO) << "Acceptor finished";

    return 0;
}

void DrdbServerApp::parse_cli() {
    int opt;
    while ((opt = getopt(argc, argv, "s:p:c:l:d:r:")) != -1) {
        switch (opt) {
            case 's':
                server_name = std::string(optarg);
                VLOG(INFO) << "Set server_name to " << server_name;
                break;
            case 'p':
                try {
                    server_port = std::stoi(optarg);
                } catch (std::exception const &e) {
                    VLOG(ERROR) << "Given server_port is not a number";
                    std::exit(-1);
                }
                VLOG(INFO) << "Set server_port to " << server_port;
                break;
            case 'r':
                try {
                    redirect_clients_pro_server = std::stoi(optarg);
                } catch (std::exception const &e) {
                    VLOG(ERROR) << "Given redirect_clients_pro_server is not a number";
                    std::exit(-1);
                }
                VLOG(INFO) << "Set redirect_clients_pro_server to " << redirect_clients_pro_server;
                break;
            case 'c':
                server_config_path = std::string(optarg);
                VLOG(INFO) << "Set server_config_path to " << server_config_path;
                break;
            case 'l':
                log_path = std::string(optarg);
                VLOG(INFO) << "Set log_path to " << log_path;
                break;
            case 'd':
                db_path = std::string(optarg);
                VLOG(INFO) << "Set db_path to " << db_path;
                break;
            case '?':
                VLOG(ERROR) << "Unknown CLI option: " << char(optopt);
                break;
            default:
                break;
        }
    }

    if (server_name.empty()) {
        VLOG(ERROR)
                        << "Use \"-s SERVER_NAME\" to provide the server_name as specified in the server_config.txt, shutting down..";
        std::exit(-1);
    }

    if (server_port == 0) {
        VLOG(ERROR)
                        << "Use \"-p SERVER_PORT\" to provide the server_port on which the acceptor should listen on, shutting down..";
        std::exit(-1);
    }
}

void DrdbServerApp::init_rocksdb() {
    std::filesystem::create_directories(db_path);
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status =
            rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        VLOG(ERROR) << status.ToString();
    }
    assert(status.ok());

}

void DrdbServerApp::read_server_config() {
    VLOG(INFO) << "READ_SERVER_CONFIG";

    std::ifstream file(server_config_path);

    if (file.is_open()) {

        std::string current_server_name;
        uint16_t current_server_port;
        int line_number = 0;
        while (file >> current_server_name >> current_server_port) {
            VLOG(INFO) << current_server_name << " " << current_server_port;

            // Set server_id
            if (current_server_name == server_name && current_server_port == server_port) {
                server_id = line_number;
            }

            line_number = line_number + 1;
        }
        file.close();
        server_count = line_number;

    } else {
        VLOG(INFO) << "Failed to open server_config";
        exit(-1);
    }

    VLOG(INFO) << "SERVER_ID: \"" << server_id << "\"";
    VLOG(INFO) << "SERVER_COUNT: \"" << server_count << "\"";

}

void DrdbServerApp::connect_to_other_servers() {
    VLOG(INFO) << "CONNECT_TO_OTHER_SERVERS";

    std::ifstream file(server_config_path);

    if (file.is_open()) {

        std::string current_server_name;
        uint16_t current_server_port;
        int line_number = 0;
        int client_id = 0;
        while (file >> current_server_name >> current_server_port) {
            VLOG(INFO) << current_server_name << " " << current_server_port;

            // Connect to every other server in the list but yourself
            if (current_server_name != server_name) {

                for (int i = 0; i < redirect_clients_pro_server; i++) {
                    VLOG(INFO) << i << " Trying to connect to Server: \"" << current_server_name << "\"";
                    redirect_clients[line_number].push_back(
                            std::make_shared<DrdbClient>(
                                    client_id,
                                    current_server_name,
                                    current_server_port,
                                    &ucp_context
                            )
                    );
                    client_id++;
                }
            }

            line_number = line_number + 1;
        }
        file.close();

    } else {
        VLOG(INFO) << "Failed to open server_config";
        exit(-1);
    }

    VLOG(ERROR) << "SERVERAPP" << " PRINTING CLIENTS_TO_SERVERS MAP";
    for (const auto &element: redirect_clients) {
        for (const auto &subelement: element.second) {
            VLOG(ERROR) << "SERVERAPP map_id: " << element.first << " drdbClient_id: " << subelement->id;
        }
    }

}

void DrdbServerApp::init_ucp_context() {
    ucp_params_t ucp_params;
    ucs_status_t status;

    memset(&ucp_params, 0, sizeof(ucp_params));

    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;

    ucp_params.features = UCP_FEATURE_STREAM;

    status = ucp_init(&ucp_params, nullptr, &this->ucp_context);
    if (status != UCS_OK) {
        VLOG(ERROR) << "Error at ucp_init " << ucs_status_string(status);
        std::exit(-1);
    }

}
