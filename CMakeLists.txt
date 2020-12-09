cmake_minimum_required(VERSION 3.17)
project(network_systems_pa4 C)

set(CMAKE_C_STANDARD 99)
find_package(OpenSSL REQUIRED)
find_package(Threads)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG)
endif ()

add_executable(dfc client.c client.h macro.h configParser.c configParser.h common.c common.h)
add_executable(dfs server.c server.h macro.h common.c common.h)
target_link_libraries(dfc OpenSSL::SSL)
target_link_libraries(dfs OpenSSL::SSL ${CMAKE_THREAD_LIBS_INIT})
