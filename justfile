default:
  just --list

init:
  cmake -S. -B cmake_build

build: init
  cmake --build cmake_build

build_release: init
  cmake --build cmake_build --config Release

test: build
  cd cmake_build && ctest --output-on-failure

run: build && exec

run_release: build_release && exec

exec:
    ./cmake_build/proyecto_estructura

clean:
  rm -rf cmake_build
