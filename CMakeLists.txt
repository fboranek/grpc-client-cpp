cmake_minimum_required(VERSION 3.12)

project(grpc-client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_INSTALL_PREFIX "/opt" CACHE INTERNAL "Install dir prefix" FORCE)

find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(gRPC REQUIRED grpc++)
find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/server.proto)

add_custom_command(
    OUTPUT
       "${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.cc"
       "${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.h"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --grpc_out ${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${CMAKE_CURRENT_SOURCE_DIR}/proto/server.proto
    DEPENDS proto/server.proto
    COMMENT "Running C++ gRPC compiler on proto/server.proto"
    VERBATIM )

# include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(protos STATIC
      ${PROTO_SRCS}
      "${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.cc"
      "${CMAKE_CURRENT_BINARY_DIR}/server.grpc.pb.h"
)
target_include_directories(protos INTERFACE ${CMAKE_CURRENT_BINARY_DIR} PUBLIC ${gRPC_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS})
target_compile_options(protos PUBLIC ${gRPC_CFLAGS_OTHER})
target_link_libraries(protos PUBLIC ${gRPC_LIBRARIES} ${Protobuf_LIBRARIES})

add_subdirectory(src)
enable_testing()
