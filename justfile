default:
  just --list

init:
  cmake -S. -B cmake_build

build: init
  cmake --build cmake_build

build_release: init
  cmake --build cmake_build --config Release

test: build
  cd cmake_build && ctest

run: build
  ./cmake_build/proyecto_estructura

run_release: build_release
  ./cmake_build/proyecto-estructura

clean:
  rm -rf cmake_build
