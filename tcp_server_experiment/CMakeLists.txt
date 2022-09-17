cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)

project(tcp_server_experiment)

link_directories(${CMAKE_SOURCE_DIR}/server/target/release/)

add_executable(tcp_server_experiment main.cpp server/target/cxxbridge/server/src/lib.rs.h server/target/cxxbridge/server/src/lib.rs.cc server/target/cxxbridge/rust/cxx.h)

target_link_libraries(tcp_server_experiment libserver.a)
