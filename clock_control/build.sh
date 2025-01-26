#!/bin/bash
echo "Building pico code"
export PICO_SDK_PATH="$(pwd)/external/pico-sdk"

# Check if build directory exists.
if [ ! -d build ]; then
  mkdir build
  cd build
  cmake ..
else
  cd build
fi

make

