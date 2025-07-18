# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD vccgnd_rp2040_16mb CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

project(rp2040_ov7670)

pico_sdk_init()

add_executable(firmware
	ov7670.c
	main.c
)

pico_generate_pio_header(firmware ${CMAKE_CURRENT_LIST_DIR}/image.pio)

target_link_libraries(firmware
	pico_stdlib
	hardware_dma
	hardware_i2c
	hardware_pio
	hardware_pwm
)
# disable usb output, enable uart output
pico_enable_stdio_usb(firmware 0)
pico_enable_stdio_uart(firmware 1)

pico_add_extra_outputs(firmware)
