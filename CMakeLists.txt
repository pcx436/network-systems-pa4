cmake_minimum_required(VERSION 3.17)
project(network_systems_pa4 C)

set(CMAKE_C_STANDARD 99)
find_package(OpenSSL REQUIRED)

add_executable(client client.c client.h)
add_executable(server server.c server.h)
target_link_libraries(client OpenSSL::SSL)
target_link_libraries(server OpenSSL::SSL)
