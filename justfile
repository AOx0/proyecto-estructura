default:
  just --list

init:
  cmake .

build: init
  cmake --build .

build_release: init 
  cmake --build . --config Release

run: build
  ./proyecto-estructura

run_release: build_release
  ./proyecto-estructura

clean:
  rm -rf cmake_install.cmake CMakeFiles CMakeCache.txt Makefile 
