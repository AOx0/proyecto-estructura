## For this to work on windows the rust lib must be compiled
## with the toolchain
##     stable-x86_64-pc-windows-gnu
## as a static lib with linked libraries listed by:
##     cargo rustc --lib -- --print native-static-libs --crate-type staticlib
##
## base mingw from clion is ok for the compilation from within CLion
## no mingw is required in the main system due to rustup shipping it with component rust-mingw

cmake_minimum_required(VERSION 3.10)
project(proyecto_estructura)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)

# Windows must be built with rust stable-x86_64-pc-windows-gnu as target
if (WIN32)
    # Windows needs dynamic libraries 100%
    set(BUILD_SHARED_LIBS ON)
    # We do want static libgcc tho
    set(CMAKE_EXE_LINKER_FLAGS -static)
else()
    # Unix doesn't
    set(BUILD_SHARED_LIBS OFF)
endif(WIN32)

# Agregamos `googltest` para poder escribir pruebas del código
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Agregamos corrosion para compilar rust por default
FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.2.1
)

FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(MANIFEST_PATH server/Cargo.toml)# PROFILE release)
corrosion_import_crate(MANIFEST_PATH fm/Cargo.toml)# PROFILE release)
corrosion_import_crate(MANIFEST_PATH serializer/Cargo.toml)# PROFILE release)


# Agregamos fmt, una libreria de formateo de strings moderna
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 9.1.0
)

FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        Result
        GIT_REPOSITORY  https://github.com/bitwizeshift/result.git
        GIT_TAG         v1.0.0
)
FetchContent_MakeAvailable(Result)

# For profiling, disable when building the release
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

enable_testing()

set(LIBRARIES
        src/lib/server.hpp
        src/lib/database.cpp
        src/lib/database.hpp
        src/lib/serializer.cpp
        src/lib/serializer.hpp
        src/lib/table.cpp
        src/lib/table.hpp
        src/lib/fm.cpp
        src/lib/fm.hpp
        src/lib/analyzer/tokenizer.cpp
        src/lib/analyzer/tokenizer.hpp 
        src/lib/analyzer/parser.cpp
        src/lib/analyzer/parser.hpp 
        src/lib/analyzer.hpp
        src/lib/analyzer/automata.cpp
        src/lib/analyzer/automata.hpp
        src/lib/logger.cpp
        src/lib/logger.hpp
        src/lib/linkedList.cpp
        src/lib/linkedList.hpp)

add_executable(proyecto_estructura
        src/main.cpp
        ${LIBRARIES}
        )

add_executable(
        proyecto_tests
        tests/server.cc # Agregamos los tests
        ${LIBRARIES}
        tests/database.cc
        tests/table.cc 
        tests/fm.cc
        tests/tokenizer..cc 
        tests/parser.cc 
        tests/automata.cc
        tests/serializer.cc
)

target_link_libraries(proyecto_tests GTest::gtest_main fmt::fmt-header-only fm Result::Result serializer)
target_link_libraries(proyecto_estructura PUBLIC tcpserver fmt::fmt-header-only fm Result::Result serializer)

include(GoogleTest)
gtest_discover_tests(proyecto_tests)
