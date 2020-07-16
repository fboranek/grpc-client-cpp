
add_executable(grpc-server main.cc)
target_link_libraries(grpc-server protos)

install(TARGETS grpc-server RUNTIME DESTINATION grpc/bin)
