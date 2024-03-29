#include "server.grpc.pb.h"
#include <csignal>
#include <cstdlib>
#include <future>
#include <grpcpp/grpcpp.h>
#include <thread>

class TestServiceImpl final : public EchoService::Service {
public:
    std::chrono::milliseconds delay{0};
    std::string echoMessage{};

    ::grpc::Status echo(
        ::grpc::ServerContext* context,
        const ::Request* request,
        ::Response* response) override
    {
        if (delay.count()) {
            std::this_thread::sleep_for(delay);
        }
        response->set_reply(echoMessage + request->message());
        return {};
    }
};

class EnvServerOption : public grpc::ServerBuilderOption {
public:
    void UpdateArguments(grpc::ChannelArguments* args) override
    {
        auto setInt = [&](const std::string& name) {
            std::string envName = name;
            std::replace(envName.begin(), envName.end(), '.', '_');
            const char* strVal = std::getenv(envName.c_str());
            if (strVal) {
                auto val = std::stoi(strVal);
                args->SetInt(name, val);
                std::cout << "GRPC server option " << name << ": " << val << std::endl;
            }
        };
        setInt(GRPC_ARG_MAX_CONNECTION_AGE_MS);
        setInt(GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS);
    }
    void UpdatePlugins(std::vector<std::unique_ptr<grpc::ServerBuilderPlugin>>*) override {}
};


std::unique_ptr<grpc::Server> grpcServer;

int main(int argc, char* argv[])
try {
    TestServiceImpl testServiceImpl;

    const char* strDelayVal = std::getenv("GRPC_SERVER_DELAY_REQUEST_MS");
    if (strDelayVal) {
        testServiceImpl.delay = std::chrono::milliseconds(std::stoi(strDelayVal));
        std::cout << "GRPC_SERVER_DELAY_REQUEST_MS: " << testServiceImpl.delay.count() << std::endl;
    }

    const char* strMessageVal = std::getenv("GRPC_SERVER_ECHO_MESSAGE");
    if (strMessageVal) {
        testServiceImpl.echoMessage = "[" + std::string{strMessageVal} + "]:";
        std::cout << "GRPC_SERVER_ECHO_MESSAGE: " << testServiceImpl.echoMessage << std::endl;
    }

    grpcServer = [&] {
        grpc::ServerBuilder builder;

        const auto grpcUri = "0.0.0.0:8442";
        builder.AddListeningPort(grpcUri, grpc::InsecureServerCredentials());
        builder.SetSyncServerOption(grpc::ServerBuilder::MIN_POLLERS, 10);
        builder.SetSyncServerOption(grpc::ServerBuilder::MAX_POLLERS, 100);
        builder.SetSyncServerOption(grpc::ServerBuilder::NUM_CQS, 1);
        builder.SetSyncServerOption(grpc::ServerBuilder::CQ_TIMEOUT_MSEC, 10'000);
        builder.SetOption(std::make_unique<EnvServerOption>());
        builder.RegisterService(&testServiceImpl);

        std::cout << "GRPC server configured listening at " << grpcUri << std::endl;
        return builder.BuildAndStart();
    }();

    std::signal(SIGINT, [](int) {
        std::async(std::launch::async, [] {
            if (grpcServer) grpcServer->Shutdown();
        }).get();
    });
    grpcServer->Wait();
}
catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
