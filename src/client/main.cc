#include "server.grpc.pb.h"
#include <fstream>
#include <grpcpp/grpcpp.h>

class GrpcClient {
public:
    int total{0};
    int success{0};

    GrpcClient(const std::vector<std::string>& addresses, std::chrono::milliseconds timeout)
    : timeout(timeout)
    {
        for (auto address : addresses) {
            grpc::ChannelArguments channelArguments;
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 1'500);
            channelArguments.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 150);

            stubs.push_back(TestService::NewStub(grpc::CreateCustomChannel(
                address, grpc::InsecureChannelCredentials(), channelArguments)));
        }
    };

    void testConnection(std::string value)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto timeDiff = [&]() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now() - start)
                .count();
        };

        ::Request request;
        request.set_message(value);

        ::grpc::CompletionQueue cq;
        std::vector<GrpcStatusContextResponse<::Response>> grpcCalls {stubs.size()};
        std::vector<std::unique_ptr<::grpc::ClientAsyncResponseReaderInterface<::Response>>> rpcs;

        // start requests to all servers
        auto deadline = std::chrono::system_clock::now() + timeout;
        for (size_t i = 0; i < stubs.size(); ++i) {
            grpcCalls[i].index = i + 1;
            grpcCalls[i].context.set_deadline(deadline);

            auto rpc = stubs[i]->Asynctest(&grpcCalls[i].context, request, &cq);
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
            std::cout << "[" << reinterpret_cast<size_t>(got_tag) << "] finished (+" << timeDiff()
                      << " ms).\n";
        }

        auto end = timeDiff();

        // do something with results
        size_t okCalls = 0;
        for (const auto& reply : grpcCalls) {
            if (reply.status.ok()) { ++okCalls; }
        }

        ++total;
        success += (okCalls / grpcCalls.size());

        std::cout << "[" << okCalls << "/" << grpcCalls.size() << "] was ok (+" << end << " ms)."
                  << std::endl;
    }

protected:
    template<typename ProtoResponse>
    struct GrpcStatusContextResponse {
        size_t index;
        grpc::Status status;
        grpc::ClientContext context;
        ProtoResponse response;
    };

    std::vector<std::unique_ptr<TestService::StubInterface>> stubs;
    std::chrono::milliseconds timeout;
};

std::unique_ptr<grpc::Server> grpcServer;

int main(int argc, char* argv[])
try {
    if (argc < 2) { throw std::runtime_error("Missing argument file with addresses."); }
    std::ifstream file(argv[1]);
    std::vector<std::string> addresses;
    for (std::string line; std::getline(file, line); ) {
        if (!line.empty() && line[0] != '#') { addresses.emplace_back(line); }
    }

    {
        std::cout << "Configured for: \n";
        int index = 0;
        for (const auto& address : addresses) {
            std::cout << " - " << index++ << ": [" << address << "]\n";
        }
        std::cout << std::endl;
    }

    GrpcClient grpcClient {addresses, std::chrono::milliseconds(20)};
    for (size_t i = 0; i < 100; ++i) {
        grpcClient.testConnection("some data");
    }
    std::cout << "Summary: \n";
    std::cout << "From " << grpcClient.total << " requests was " << grpcClient.success
              << " received response from all server. \n";
}
catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
