cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

# For profiling, disable when building the release
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

project(tcp_server_experiment)

link_directories(${CMAKE_SOURCE_DIR}/server/target/release/)

add_executable(tcp_server_experiment main.cpp)

target_link_libraries(tcp_server_experiment libserver.a)
