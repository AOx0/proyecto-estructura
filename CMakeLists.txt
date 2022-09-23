## For this to work on windows the rust lib must be compiled
## with the toolchain
##     stable-x86_64-pc-windows-gnu
## as a static lib with linked libraries listed by:
##     cargo rustc --lib -- --print native-static-libs --crate-type staticlib
##
## base mingw from clion is ok for the compilation from within CLion
## no mingw is required in the main system due to rustup shipping it with component rust-mingw

cmake_minimum_required(VERSION 3.10)
project(proyecto-estructura)

set(CMAKE_CXX_STANDARD 20)

# For profiling, disable when building the release
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif (WIN32)

link_directories(${CMAKE_SOURCE_DIR}/server/target/release/)

add_executable(proyecto-estructura
  src/main.cpp 
  src/lib/option.hpp
  src/lib/result.hpp
  src/lib/server.hpp
)


if (WIN32)
    target_link_libraries(proyecto-estructura -static-libgcc -static-libstdc++ -lserver -lntdll -ladvapi32 -luserenv -lkernel32 -lkernel32 -lws2_32 -lbcrypt)
else()
    target_link_libraries(proyecto-estructura libserver.a)
endif (WIN32)
