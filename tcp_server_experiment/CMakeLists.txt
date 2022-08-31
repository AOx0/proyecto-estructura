cmake_minimum_required(VERSION 3.10)

project(tcp_server_experiment)

add_executable(tcp_server_experiment main.cpp)

target_link_libraries(tcp_server_experiment ${CMAKE_SOURCE_DIR}/server/target/release/libserver.a)
