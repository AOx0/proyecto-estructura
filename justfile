default:
  just --list

init:
  cmake -S. -B cmake-build-debug -GNinja

build nucleos="4": init
  cmake --build cmake-build-debug -j {{nucleos}}

build-release nucleos="4": init
  cmake --build cmake-build-debug -j {{nucleos}} --config Release

test nucleos="4": (build nucleos)
  cd cmake-build-debug && ctest --output-on-failure

run nucleos="4": (build nucleos) && exec

run-release nucleos="4": (build-release nucleos) && exec

exec:
    ./cmake-build-debug/toidb

clean:
  rm -rf cmake-build-debug
