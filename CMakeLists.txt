cmake_minimum_required(VERSION 3.10)
project(proyecto-estructura)

set(CMAKE_CXX_STANDARD 14)

add_executable(proyecto-estructura
  src/main.cpp 
  src/lib/option.hpp
  src/lib/result.hpp
)
