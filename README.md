# Smart Alarm clock project

NOTE: This build is intended to run in Linux. If you are running a Windows machine, you can use WSL.

## Building the project

### 1. Pull the repository
```
git clone --recurse-submodules https://github.com/DawsonTheroux/alarm_clock.git && cd alarm_clock/
```

### 2. Install the build tools
```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

### 3. Build using bash
```
./build.sh
```

### 4. Copy the artifact to the PICO
 Plug in the PICO while holding the boot button then copy the build/alarm_clock_control.uf2 file to the board.
```
cp clock_control/build/alarm_clock_control.uf2 <pico location>
```
 
