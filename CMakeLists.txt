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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)

project(rp2040_ov7670_TestProj)

pico_sdk_init()

add_executable(rp2040_ov7670_TestProj
	ov7670.c
	main.c
)

pico_generate_pio_header(rp2040_ov7670_TestProj ${CMAKE_CURRENT_LIST_DIR}/image.pio)

target_link_libraries(rp2040_ov7670_TestProj
	pico_stdlib
	hardware_dma
	hardware_i2c
	hardware_pio
	hardware_pwm
)

```
作用：禁用 USB 作为标准输入/输出（stdio）的通信接口。
参数 0：表示禁用 USB 的 stdio 功能。
典型场景：
当不需要通过 USB 输出调试信息（如 printf/log）时禁用，可以节省代码空间和资源。
```
pico_enable_stdio_usb(rp2040_ov7670_TestProj 0)
```
作用：启用 UART（串口）作为标准输入/输出的通信接口。
参数 1：表示启用 UART 的 stdio 功能。
```
pico_enable_stdio_uart(rp2040_ov7670_TestProj 1)

pico_add_extra_outputs(rp2040_ov7670_TestProj)
