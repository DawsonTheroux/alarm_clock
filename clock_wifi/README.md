# NOTE: At the moment this is a copy of the example blink program:
```
https://github.com/espressif/esp-idf/tree/master/examples/get-started/blink
```
# Building the ESP project
### 1. source the build environment. On windows, run the ESP-IDF powershell
### 2. Set the board type
```
idf.py set-target esp32c2
```
### 3. Update wifi SSID and password 
Update the default config for the wifi SSID and password by updating the defines in main/inc/wifi_secrets.h
### 4. Build the ESP8684 project
```
idf.py build
```
### 5. Flash the board
```
idf.py -p <BOARD COM PORT> flash 
```
### 6. Monitor the console
```
idf.py -p <BOARD COM PORT> monitor
```
