# ESP-Components

This is a collection of ESP32 components implemented in C++20. Some are just wrappers of the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) provide by Espressif, others are more complex implementations aiming a more _friendly_ / _less error prone_ use.

All components are provided at `components` directory. At `examples` you can see some examples of how to use this libraries. Some other facility scripts can be found at `scripts` directory.

The below commands assume that you already installed the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) and included at path.

## Adding to your project

First, clone this repository or add as submodule. Then, add to `EXTRA_COMPONENT_DIRS` variable the `components` directory of this project.

To add as a submodule:
```bash
# Adding this project as dependency to esp-components directory
$ git submodule add https://github.com/rnascunha/esp-components.git
$ cd esp-components
$ git submodule update --init --recursive
```
The `CMakeLists.txt` of your root directory must add the `components` directory, and will look like this:

```CMake
# At CMakeLists.txt of the root of your project:
cmake_minimum_required(VERSION 3.16)

# Add this line
set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} "${CMAKE_SOURCE_DIR}/esp-components/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(project-name)
```
If you already have `esp-components` as submodule, and want to update from server, type at root directory:
```bash
$ git submodule update --remote
```

## Examples

To **build** / **flash** / **monitor** a single example:
```
idf.py -C <example-directory> [build] [flash] [monitor]
```
So, if you are at the root directory, to compile, flash and monitor the `echo_http_ws_serial` example:
```
$ idf.py -C examples/echo_http_ws_serial/ build flash monitor
```
To **build** all examples you can use the `examples/run.sh` script:
```
$ ./run.sh <root-directory> [1]
```
Where `<root-directory>` is the root directory of this project, and `1`, if defined, will make a fullclean (`idf.py fullclean`) before trying build the project (if not set or set something different of `1` it will make a normal `clean``). For example, if you are at the root directory:
```
$ ./examples/run.sh . 1
----------------------------RUNNING EXAMPLES-----------------------------
Date: 2023/09/12 15:40:01
IDF_Version: ESP-IDF v5.2-dev-1962-g53ff7d43db
IDF_PATH: /home/rnascunha/dev/toolchain/esp-idf/esp-idf-v5.2-dev-1962-g53ff7d43db
Command: ./examples/run.sh . 1
Fullclean: 1
-------------------------------------------------------------------------
+-----+-------------------------------------+----------+------------------+---------+
|  Op |             project name            |  status  | time(s.ms.us.ns) | size(b) |
+-----+-------------------------------------+----------+------------------+---------+
|CLEAN| log ................................  SUCCESS
|BUILD| log ................................  SUCCESS  |  26.783.917.380  | 277584
|CLEAN| uc/adc_stream ......................  SUCCESS
|BUILD| uc/adc_stream ......................  SUCCESS  |  26.591.032.844  | 269664
|CLEAN| uc/gpio ............................  SUCCESS
|BUILD| uc/gpio ............................  SUCCESS  |  24.229.050.908  | 183328
|CLEAN| uc/pulse_counter ...................  SUCCESS
|BUILD| uc/pulse_counter ...................  SUCCESS  |  26.383.319.378  | 180240
|CLEAN| peripherals/water_flow_sensor ......  SUCCESS
|BUILD| peripherals/water_flow_sensor ......  SUCCESS  |  26.867.080.090  | 175728
|CLEAN| wave ...............................  SUCCESS
|BUILD| wave ...............................  SUCCESS  |  24.377.128.491  | 265472
|CLEAN| websocket/server ...................  SUCCESS
|BUILD| websocket/server ...................  SUCCESS  |  27.994.910.568  | 791792
|CLEAN| wifi/station .......................  SUCCESS
|BUILD| wifi/station .......................  SUCCESS  |  24.335.970.833  | 815488
|CLEAN| wifi/ap ............................  SUCCESS
|BUILD| wifi/ap ............................  SUCCESS  |  24.590.187.797  | 816608
|CLEAN| http/server ........................  SUCCESS
|BUILD| http/server ........................  SUCCESS  |  27.613.305.380  | 786624
|CLEAN| http/server_cb .....................  SUCCESS
|BUILD| http/server_cb .....................  SUCCESS  |  27.465.016.971  | 786656
|CLEAN| http/login_wifi ....................  SUCCESS
|BUILD| http/login_wifi ....................  SUCCESS  |  30.756.227.905  | 816048
|CLEAN| http/https_server ..................  SUCCESS
|BUILD| http/https_server ..................  SUCCESS  |  28.704.901.314  | 788624
|CLEAN| echo_http_ws_serial ................  SUCCESS
|BUILD| echo_http_ws_serial ................  SUCCESS  |  27.838.748.110  | 806640
------------------------------------------------------------
Success:  log uc/adc_stream uc/gpio uc/pulse_counter peripherals/water_flow_sensor wave websocket/server wifi/station wifi/ap http/server http/server_cb http/login_wifi http/https_server echo_http_ws_serial
Fail   : 
```

## Reference

* https://docs.espressif.com/projects/esp-idf/en/latest/esp32/
