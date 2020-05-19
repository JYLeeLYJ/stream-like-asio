
add_executable(asio_client client.cpp)
set_lib_link(asio_client)

add_executable(asio_server server.cpp)
set_lib_link(asio_server)