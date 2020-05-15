cmake_minimum_required(VERSION 3.15)
project(EchoServer)

set(CMAKE_CXX_STANDARD 17)


set (Boost_USE_STATIC_LIBS OFF)
set (Boost_USE_MULTITHREAD ON)

find_package(Boost REQUIRED COMPONENTS system thread)


include_directories(${Boost_INCLUDE_DIRS})
#link_directories(${Boost_LIBRARY_DIRS})


add_executable(server boost_default.cpp)


target_link_libraries(server ${Boost_LIBRARIES})
target_link_libraries(server ws2_32 wsock32)

add_executable(client boost_client.cpp)
target_link_libraries(client ${Boost_LIBRARIES})
target_link_libraries(client ws2_32 wsock32)

