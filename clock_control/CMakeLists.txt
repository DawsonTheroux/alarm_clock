cmake_minimum_required(VERSION 3.19)

# initialize the SDK based on PICO_SDK_PATH
include(pico_sdk_import.cmake)
project(alarm_clock_control)

# initialize the Pico SDK
pico_sdk_init()

add_executable(alarm_clock_control
    src/main.c
    src/alarm_clock.c
)

# Uncomment the following line to add a PIO program.
# pico_generate_pio_header(alarm_clock_control ${CMAKE_CURRENT_LIST_DIR}/src/alarm_clock.pio)

FILE(GLOB FreeRTOS_src ../external/FreeRTOS-Kernel/*.c)

add_library( FreeRTOS STATIC
	${FreeRTOS_src}
	../external/FreeRTOS-Kernel/portable/GCC/ARM_CM0/port.c
	../external/FreeRTOS-Kernel/portable/MemMang/heap_4.c
)

target_include_directories( FreeRTOS PUBLIC
		../external/FreeRTOS-Kernel/include
		include/
		../external/FreeRTOS-Kernel/portable/GCC/ARM_CM0
	)

target_link_libraries(alarm_clock_control
	pico_stdlib 
	hardware_gpio 
	hardware_pio
	FreeRTOS
	)

	
pico_add_extra_outputs(alarm_clock_control)
