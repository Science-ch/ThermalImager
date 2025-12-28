# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "D:/Libraries/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(ThermalImager C CXX ASM)

include_directories("include")
file(GLOB C_SOURCES "src/*.c")

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(FREERTOS_KERNEL_PATH "D:/Libraries/FreeRTOS-KernelV11.2.0" CACHE PATH "FreeRTOS Kernel path")

if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Skipping FreeRTOS examples as FREERTOS_KERNEL_PATH not defined")
    return()
endif()

include(FreeRTOS_Kernel_import.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(ThermalImager ${C_SOURCES})

pico_set_program_name(ThermalImager "ThermalImager")
pico_set_program_version(ThermalImager "0.1")

pico_enable_stdio_uart(ThermalImager 1)
pico_enable_stdio_usb(ThermalImager 0)

# Add the standard library to the build
target_link_libraries(ThermalImager
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(ThermalImager PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(ThermalImager 
        hardware_spi
        hardware_adc
        hardware_i2c
        hardware_dma
        hardware_pwm
        hardware_pio
        hardware_timer
        hardware_clocks
        pico_async_context_freertos
        FreeRTOS-Kernel-Heap4
        )

# Set the nunber of cores to 1.
# This defaults to 2 in FreeRTOSConfig_examples_common.h if not defined in here
target_compile_definitions(ThermalImager PRIVATE
    configNUMBER_OF_CORES=1
    )
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(ThermalImager PRIVATE
        pico_cyw43_arch_none
        )
endif()
pico_add_extra_outputs(ThermalImager)
