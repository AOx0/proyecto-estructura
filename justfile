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

test: build clean
  ctest

run: build
  ./proyecto-estructura

run_release: build_release
  ./proyecto-estructura

clean_all: clean clean_tests

clean:
  rm -rf cmake_install.cmake CMakeFiles CMakeCache.txt Makefile bin _deps lib Testing

clean_tests:
  rm -rf CTestTestfile.cmake proyecto_tests[1]_include.cmake proyecto_tests[1]_tests.cmake