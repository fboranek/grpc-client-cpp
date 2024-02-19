
add_executable(grpc-client  main.cc)
target_link_libraries(grpc-client protos Boost::program_options)

install(TARGETS grpc-client RUNTIME DESTINATION bin)
