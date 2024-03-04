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

    void testConnectionIndex(size_t index)
    {
        if (stubs.size() > index) {
            auto now = std::chrono::system_clock::now();
            grpc::ClientContext context;
            context.set_deadline(now + timeout);

            ::Request request;
            request.set_message(std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()));
            ::Response response;
            auto res = stubs[index]->echo(&context, request, &response);
            if (res.ok()) {
                std::cout << index << " [ok]: " << response.reply() << std::endl;
            } else {
                std::cout << index << " [fail]: " << res.error_message() << std::endl;
            }
        } else {
            std::cout << index << " [fail]: no address for index" << index << std::endl;
        }
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
        ("address-file", po::value<std::string>()->required(), "Text file with addess on each line. Can be as first positional argument.");
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
        std::cout << "Configured as deamon for: \n";
        int index = 0;
        for (const auto& address : addresses) {
            std::cout << " - " << index++ << ": [" << address << "]\n";
        }
        std::cout << std::endl;
    }

    GrpcClient grpcClient{addresses, std::chrono::milliseconds(500)};
    {
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        const auto interval        = std::chrono::minutes{60};
        const auto subInterval     = std::chrono::minutes{1};
        const auto counterTrashold = interval / subInterval;

        std::cout << "Started to make requests each " << subInterval << " up to " << interval
                  << std::endl;

        grpcClient.testConnectionIndex(0);
        grpcClient.testConnectionIndex(1);
        std::this_thread::sleep_for(std::chrono::seconds{1});

        int counter = 0;
        while (!shouldTerminate) {
            grpcClient.testConnectionIndex(0);
            if (counter == counterTrashold) {
                std::cout << " + " << interval << " ....." << std::endl;
            }
            if (counter >= counterTrashold) {
                grpcClient.testConnectionIndex(1);
            }
            ++counter;
            std::this_thread::sleep_for(subInterval);
        }
        std::cout << "\nTerminated\n";
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
