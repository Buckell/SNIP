cmake_minimum_required(VERSION 3.23)
project(snip_client1)
project(snip_client2)
project(snip_server)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "C:/dev/boost_1_80_0/"
)

add_executable(snip_client1 client1_test.cpp)
add_executable(snip_client2 client2_test.cpp)
add_executable(snip_server server_test.cpp)