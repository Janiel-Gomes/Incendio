# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(incendio C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(incendio 
    incendio.c 
    firmware/src/aht10.c
)

# FreeRTOS setup
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/firmware/lib/FreeRTOS-Kernel)

# Add FreeRTOS Kernel source files
target_sources(incendio PRIVATE
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
    ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    # Also need sys_arch for LwIP/FreeRTOS integration
    ${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/sys_arch.c
)

# Add includes manually since SDK detection is failing
target_include_directories(incendio PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/firmware/include
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM33_NTZ/non_secure
    ${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include
)

pico_set_program_name(incendio "incendio")
pico_set_program_version(incendio "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(incendio 1)
pico_enable_stdio_usb(incendio 1)

# Add the standard library to the build
target_link_libraries(incendio
        pico_stdlib
        hardware_i2c
        pico_cyw43_arch_lwip_sys_freertos
)

pico_add_extra_outputs(incendio)
