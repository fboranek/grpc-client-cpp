#include "server.grpc.pb.h"
#include <boost/program_options.hpp>
#include <csignal>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <thread>

namespace {

class GrpcClient {
public:
    int total{0};
    int success{0};

    GrpcClient(const std::vector<std::string>& addresses, std::chrono::milliseconds timeout)
        : timeout(timeout)
    {
        for (auto address : addresses) {
            grpc::ChannelArguments channelArguments;
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 5000);
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 1000);

            stubs.push_back(EchoService::NewStub(grpc::CreateCustomChannel(
                address,
                grpc::InsecureChannelCredentials(),
                channelArguments)));
        }
    };

    void testConnection(std::string value)
    {
        auto start    = std::chrono::high_resolution_clock::now();
        auto timeDiff = [&]() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now() - start)
                .count();
        };

        ::Request request;
        request.set_message(value);

        ::grpc::CompletionQueue cq;
        std::vector<GrpcStatusContextResponse<::Response>> grpcCalls{stubs.size()};
        std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReaderInterface<::Response>>> rpcs;

        // start requests to all servers
        auto deadline = std::chrono::system_clock::now() + timeout;
        for (size_t i = 0; i < stubs.size(); ++i) {
            grpcCalls[i].context.set_deadline(deadline);

            auto rpc = stubs[i]->Asyncecho(&grpcCalls[i].context, request, &cq);
            rpc->Finish(&grpcCalls[i].response, &grpcCalls[i].status, reinterpret_cast<void*>(i));
            rpcs.push_back(std::move(rpc));
        }

        // mark queue as finished
        cq.Shutdown();
        std::cout << "signal shutdown (+" << timeDiff() << " ms).\n";

        // wait on all responses
        void* got_tag;
        bool ok = false;
        while (cq.Next(&got_tag, &ok)) {
            const auto index     = reinterpret_cast<size_t>(got_tag);
            const auto& response = grpcCalls[index].response;
            std::cout << "[" << index << "] finished (+" << timeDiff()
                      << " ms): " << response.reply() << "\n";
        }

        auto end = timeDiff();

        // do something with results
        size_t okCalls = 0;
        for (const auto& reply : grpcCalls) {
            if (reply.status.ok()) {
                ++okCalls;
            }
        }

        ++total;
        success += (okCalls / grpcCalls.size());

        std::cout << "[" << okCalls << "/" << grpcCalls.size() << "] was ok (+" << end << " ms)."
                  << std::endl;
    }

protected:
    template<typename ProtoResponse>
    struct GrpcStatusContextResponse {
        grpc::Status status;
        grpc::ClientContext context;
        ProtoResponse response;
    };

    std::vector<std::unique_ptr<EchoService::StubInterface>> stubs;
    std::chrono::milliseconds timeout;
};

std::atomic<bool> shouldTerminate = false;
void handleSignal(int)
{
    shouldTerminate = true;
}
} // namespace

int main(int argc, char* argv[])
try {
    namespace po = boost::program_options;

    po::options_description desc("Usage");
    // clang-format off
    desc.add_options()
        ("help,h", "Produce help message.")
        ("address-file", po::value<std::string>()->required(), "Text file with addess on each line. Can be as first positional argument.")
        ("daemon,d", po::bool_switch()->default_value(false), "Run as deamon.")
        ("interval,i", po::value<std::uint32_t>()->default_value(500), "Interval between requests in deaemon mode.");
    // clang-format on
    po::positional_options_description p;
    p.add("address-file", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    if (vm.count("help")) {
        std::cout << "Grpc client calling echo service to test gRPC communication."
                  << "\n";
        std::cout << desc << "\n";
        return EXIT_FAILURE;
    }
    po::notify(vm); // throw if required options are missing

    const auto& addressFile = vm["address-file"].as<std::string>();
    const auto& daemon      = vm["daemon"].as<bool>();
    const auto interval     = std::chrono::milliseconds{vm["interval"].as<std::uint32_t>()};


    auto addresses = [&] {
        std::vector<std::string> addresses;
        std::ifstream file(addressFile);
        for (std::string line; std::getline(file, line);) {
            if (!line.empty() && line[0] != '#') {
                addresses.emplace_back(line);
            }
        }
        return addresses;
    }();
    if (addresses.empty()) {
        std::cout << "File '" << addressFile << "' do not contains any address\n";
        return EXIT_FAILURE;
    }

    {
        std::cout << "Configured as " << (daemon ? "deamon" : "job") << " for: \n";
        int index = 0;
        for (const auto& address : addresses) {
            std::cout << " - " << index++ << ": [" << address << "]\n";
        }
        std::cout << std::endl;
    }

    GrpcClient grpcClient{addresses, std::chrono::milliseconds(20)};
    if (daemon) {
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        std::cout << "Started to make requests each " << interval << " ...\n";
        while (!shouldTerminate) {
            grpcClient.testConnection("some data");
            std::this_thread::sleep_for(std::chrono::milliseconds{interval});
        }
        std::cout << "\nTerminated\n";
    } else {
        for (size_t i = 0; i < 100; ++i) {
            grpcClient.testConnection("some data");
        }
    }
    std::cout << "Summary: \n";
    std::cout << "From " << grpcClient.total << " requests was " << grpcClient.success
              << " received response from all server. \n";
    return EXIT_SUCCESS;
}
catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
