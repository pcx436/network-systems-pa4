cmake_minimum_required(VERSION 3.17)
project(network_systems_pa4 C)

set(CMAKE_C_STANDARD 99)
find_package(OpenSSL REQUIRED)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG)
endif ()

add_executable(dfc client.c client.h macro.h)
add_executable(dfs server.c server.h macro.h)
target_link_libraries(dfc OpenSSL::SSL)
target_link_libraries(dfs OpenSSL::SSL)
