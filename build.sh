#!/bin/bash
echo "Building pico code"
export PICO_SDK_PATH="$(pwd)/external/pico-sdk"
cd clock_control

# Check if build directory exists.
if [ ! -d build ]; then
  mkdir build
  cd build
  cmake ..
else
  cd build
fi

make
echo "Clock control build complete."
echo "Plug in the matrix while holding the boot select pin and run the following command:"
echo "    cp clock_control/build/alarm_clock_control.uf2 E:\\"

