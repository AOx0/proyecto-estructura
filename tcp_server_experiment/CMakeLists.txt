## For this to work on windows the rust lib must be compiled
## with the toolchain
##     stable-x86_64-pc-windows-gnu
## as a static lib with linked libraries listed by:
##     cargo rustc --lib -- --print native-static-libs --crate-type staticlib
##
## base mingw from clion is ok for the compilation from within CLion
## no mingw is required in the main system due to rustup shipping it with component rust-mingw

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 14)

# For profiling, disable when building the release
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
set(CMAKE_EXE_LINKER_FLAGS "-static")

project(tcp_server_experiment)

link_directories(${CMAKE_SOURCE_DIR}/server/target/release/)

add_executable(tcp_server_experiment main.cpp)

target_link_libraries(tcp_server_experiment -lserver -lntdll -ladvapi32 -luserenv -lkernel32 -lkernel32 -lws2_32 -lbcrypt)
