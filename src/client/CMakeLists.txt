
add_executable(grpc-client  main.cc)
target_link_libraries(grpc-client protos)

install(TARGETS grpc-client RUNTIME DESTINATION grpc/bin)

# install(DIRECTORY DESTINATION grpc-server/log/archive)
# install(FILES conf/.conf DESTINATION grpc-server/conf)s