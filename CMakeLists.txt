cmake_minimum_required(VERSION 3.16)
project(echo_server_ars)

set(CMAKE_CXX_STANDARD 14)

add_executable(echo_server_ars
        client.cpp
        server.cpp)
