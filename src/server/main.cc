#include "server.grpc.pb.h"
#include <csignal>
#include <grpcpp/grpcpp.h>

class TestServiceImpl final : public TestService::Service {
public:
    ::grpc::Status test(::grpc::ServerContext* context,
                        const ::Request* request,
                        ::Response* response) override
    {
        response->set_message(request->message());
        return {};
    }
};

std::unique_ptr<grpc::Server> grpcServer;

int main(int argc, char* argv[])
try {
    TestServiceImpl testServiceImpl;

    grpcServer = [&] {
        grpc::ServerBuilder builder;

        const auto grpcUri = "0.0.0.0:8442";
        builder.AddListeningPort(grpcUri, grpc::InsecureServerCredentials());
        builder.SetSyncServerOption(grpc::ServerBuilder::MIN_POLLERS, 10);
        builder.SetSyncServerOption(grpc::ServerBuilder::MAX_POLLERS, 100);
        builder.SetSyncServerOption(grpc::ServerBuilder::NUM_CQS, 1);
        builder.SetSyncServerOption(grpc::ServerBuilder::CQ_TIMEOUT_MSEC, 10'000);
        builder.RegisterService(&testServiceImpl);

        std::cout << "GRPC server started listening at " << grpcUri << std::endl;
        return builder.BuildAndStart();
    }();

    std::signal(SIGINT, [](int) {
        if (grpcServer) grpcServer->Shutdown();
    });
    grpcServer->Wait();
}
catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
