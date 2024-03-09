# NOTE: At the moment this is a copy of the example blink program:
```
https://github.com/espressif/esp-idf/tree/master/examples/get-started/blink
```
# Smart Alarm clock project

NOTE: Right now, the PICO is built under WSL and the ESP8684 is built on Windows. I am going to look into getting the pico to build on windows.

## Setting up the environment
### Installing the RP2040 environment dependencies (LINUX)
```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```
### Installing ESP8684 dependencies
Download and install ESP-IDF. Be sure to include the ESP32-C2 SDK
```
https://docs.espressif.com/projects/esp-idf/en/latest/esp32c2/get-started/index.html
```

## Building the RP2040 code
### 1. Pull the repository
```
git clone --recurse-submodules https://github.com/DawsonTheroux/alarm_clock.git && cd alarm_clock/
```

### 3. Build the pico code
```
cd clock_control
./build.sh
```

### 4. Copy the artifact to the PICO
 Plug in the PICO while holding the boot button then copy the clock_control/build/alarm_clock_control.uf2 file to the board.

## Building the ESP8684 code
### Start the ESP-IDF powershell
This powershell executable was included in the ESP-IDF install, then navigate to clock_wifi
```
cd clock_wifi
```

### Set the build target board
```
idf.py set-target esp32c2
```

### Build the ESP-IDF project
```
idf.py build
```

### Program the ESP8684 board
```
idf.py -p <BOARD COM PORT> flash
```
 
