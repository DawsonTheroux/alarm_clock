cmake_minimum_required(VERSION 3.19)

SET(project_name "alarm_clock_control")
SET(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/FreeRTOS-Kernel")
# initialize the SDK based on PICO_SDK_PATH
include(pico_sdk_import.cmake)
# project(alarm_clock_control C CXX ASM)
project(${project_name} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Pico SDK
pico_sdk_init()

SET(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE STRING "Local Config")
include_directories(${FREERTOS_CONFIG_FILE_DIRECTORY})
include(FreeRTOS_Kernel_import.cmake)

add_compile_definitions(
  FREE_RTOS_KERNEL_SMP=1
  portSUPPORT_SMP=1
)


add_executable(alarm_clock_control
    src/main.c
    src/chipcomms_i2c_host.c
    src/chipcomms_spi_device.c
    src/time_keeper.c
    src/display.c
)

pico_add_extra_outputs(alarm_clock_control)
#pico_enable_stdio_uart(alarm_clock_control 0)


target_include_directories(alarm_clock_control PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  include/
  ../common/
  )


#add_library(FreeRTOS STATIC
#    ../external/FreeRTOS-Kernel/portable/MemMang/heap_4.c
#)

target_link_libraries(alarm_clock_control 
  pico_stdlib 
  FreeRTOS-Kernel-Heap4
  hardware_gpio
  hardware_pio
  hardware_i2c
  hardware_spi
  hardware_dma
  hardware_rtc
)


target_include_directories(alarm_clock_control PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../external/FreeRTOS-Kernel/include
)


target_compile_definitions(alarm_clock_control PRIVATE
    FREE_RTOS_KERNEL_SMP=1
    portSUPPORT_SMP=1
)
	
