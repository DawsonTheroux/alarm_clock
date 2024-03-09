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
### 3. Build the project
```
idf.py build
```
### 4. Flash the board
```
idf.py -p <BOARD COM PORT> flash 
```
### 5. Monitor the console
```
idf.py -p <BOARD COM PORT> monitor
```
