default:
  just --list

init:
  cmake .

cargo_build_release:
  cd server && cargo build --release

build: init cargo_build_release
  cmake --build .

build_release: init cargo_build_release
  cmake --build . --config Release

run: build
  ./proyecto-estructura

run_release: build_release
  ./proyecto-estructura

clean:
  rm -rf cmake_install.cmake CMakeFiles CMakeCache.txt Makefile 
