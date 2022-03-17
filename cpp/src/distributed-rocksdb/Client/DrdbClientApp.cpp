#include <random>
#include "DrdbClientApp.h"

int DrdbClientApp::start() {
    // log dir may only be set before init
    // FLAGS_log_dir = "/some/log/directory";
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_minloglevel = 0;
    FLAGS_v = 5;
    VLOG(ERROR) << "Starting Client App.";

    parse_cli();

    if (start_endless_loop) {
        VLOG(ERROR) << "Starting endless loop amount: " << benchmark_object_count << " size: " << benchmark_object_size;
        FLAGS_v = 3;
        benchmark_singlethreaded_endless();
    }

    if (start_interactive_cli) {
        VLOG(ERROR) << "Starting interactive CLI";
        interactive_cli();
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        FLAGS_v = 3;
        VLOG(ERROR) << "Benchmarking amount: " << benchmark_object_count << " size: " << benchmark_object_size;
        benchmark_multithreaded();
    }

    return 0;
}

void DrdbClientApp::benchmark_singlethreaded() {
    DrdbClient drdbClient{0, server_name, server_port};

//    auto thread = drdbClient.benchmark_thread(100000, 5);
//    auto thread = drdbClient.benchmark_unknown_keys_thread(1000, 5);
    auto thread = drdbClient.benchmark_put_thread(
            benchmark_object_count, benchmark_object_size);
    thread.join();

}

[[noreturn]] void DrdbClientApp::benchmark_singlethreaded_endless() {
    DrdbClient drdbClient{0, server_name, server_port};

    int iteration = 0;
    while (true) {
        VLOG(ERROR) << "--------------------ITERATION " << iteration << "--------------------";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto thread = drdbClient.benchmark_thread(
                benchmark_object_count, benchmark_object_count);
        thread.join();
        iteration++;
    }

}

void DrdbClientApp::benchmark_singlethreaded_unknown_keys() {
    DrdbClient drdbClient{0, server_name, server_port};

    auto thread = drdbClient.benchmark_unknown_keys_thread(1000, 200);
    thread.join();

}

void DrdbClientApp::benchmark_multithreaded() {
    int thread_count = 16;
    VLOG(ERROR) << "Creating " << thread_count << " Clients";

    std::vector<std::shared_ptr<DrdbClient>> clients{};
    for (int i = 0; i < thread_count; i++) {
        clients.push_back(std::make_shared<DrdbClient>(i, server_name, server_port));
    }

    VLOG(ERROR) << "Starting Benchmark";
    std::vector<std::thread> threads{};
    for (const auto &client: clients) {
        threads.emplace_back(client->benchmark_thread(
                benchmark_object_count, benchmark_object_size));
    }

    for (auto &thread: threads) {
        thread.join();
    }

    VLOG(ERROR) << "Done";
}

void DrdbClientApp::test_thread_creation() {
    int thread_count = 10;
    VLOG(INFO) << "Creating " << thread_count << " Clients";

    std::vector<std::shared_ptr<DrdbClient>> clients{};
    for (int i = 0; i < thread_count; i++) {
        clients.push_back(std::make_shared<DrdbClient>(i, server_name, server_port));
    }

    VLOG(INFO) << "Done";
}

void DrdbClientApp::parse_cli() {
    int opt;
    while ((opt = getopt(argc, argv, "s:p:xyc:l:")) != -1) {
        switch (opt) {
            case 's':
                server_name = std::string(optarg);
                VLOG(INFO) << "Set server_name to " << server_name;
                break;
            case 'p':
                try {
                    server_port = std::stoi(optarg);
                } catch (std::exception const &e) {
                    LOG(ERROR) << "Given server_port is not a number";
                    std::exit(-1);
                }
                VLOG(INFO) << "Set server_port to " << server_port;
                break;
            case 'c':
                try {
                    benchmark_object_count = std::stoi(optarg);
                } catch (std::exception const &e) {
                    LOG(ERROR) << "Given benchmark_object_count is not a number";
                    std::exit(-1);
                }
                VLOG(INFO) << "Set server_port to " << server_port;
                break;
            case 'l':
                try {
                    benchmark_object_size = std::stoi(optarg);
                } catch (std::exception const &e) {
                    LOG(ERROR) << "Given benchmark_object_size is not a number";
                    std::exit(-1);
                }
                VLOG(INFO) << "Set server_port to " << server_port;
                break;
            case 'x':
                start_interactive_cli = true;
                break;
            case 'y':
                start_endless_loop = true;
                break;
            case '?':
                LOG(ERROR) << "Unknown CLI option: " << char(optopt);
                break;
            default:
                break;
        }
    }

    if (server_name.empty()) {
        LOG(ERROR)
                << "Use \"-s SERVER_NAME\" to provide the server_name, shutting down..";
        std::exit(-1);
    }

    if (server_port == 0) {
        LOG(ERROR)
                << "Use \"-p SERVER_PORT\" to provide the server_port, shutting down..";
        std::exit(-1);
    }
}

// Input has to be like
// METHOD KEY VALUE
void DrdbClientApp::interactive_cli() {
    DrdbClient drdbClient{0, server_name, server_port};
    std::string line;

    while (true) {
        VLOG(INFO) << "Waiting for input..";
        std::getline(std::cin, line);
        auto line_split = split(line, ' ');

        if (line_split.empty()) {
            VLOG(INFO) << "Empty input, can not process";
            continue;
        }

        auto method = resolveMethod(line_split.at(0));
        std::string key;
        std::string value;

        if (method != EXT) {
            try {
                key = line_split.at(1);
            } catch (const std::exception &e) {
                VLOG(INFO) << "Empty KEY, can not process";
                continue;
            }
        }

        if (method == PUT) {
            try {
                value = line_split.at(2);
            } catch (const std::exception &e) {
                VLOG(INFO) << "Empty VALUE, can not process";
                continue;
            }
        }

        request_reply reply;
        switch (method) {
            case GET:
                reply = drdbClient.get(key);
                break;
            case PUT:
                reply = drdbClient.put(key, value.data(), value.length());
                break;
            case DEL:
                reply = drdbClient.del(key);
                break;
            case PING:
                VLOG(INFO) << "Not yet implemented";
                continue;
            case EXT:
                reply = drdbClient.exit();
                return;
            case INVALID:
                VLOG(INFO) << R"(Invalid Command, valid commands are: "get" "put" "del" "ext")";
                continue;
        }

        VLOG(INFO) << "Status: " << DrdbStatusString(reply.drdbStatus) << " Value: " << reply.value;

    }
}
