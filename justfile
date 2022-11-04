default:
  just --list

init:
  cmake -S. -B cmake-build

build nucleos="4": init
  cmake --build cmake-build -j {{nucleos}}

build-release nucleos="4": init
  cmake --build cmake-build -j {{nucleos}} --config Release

test nucleos="4": (build nucleos)
  cd cmake-build && ctest --output-on-failure

run nucleos="4": (build nucleos) && exec

run-release nucleos="4": (build-release nucleos) && exec

exec:
    ./cmake-build/proyecto-estructura

clean:
  rm -rf cmake-build
