cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)

include(pico_sdk_import.cmake)

set(target_proj supra)
project(${target_proj} C CXX ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wl,--gc-sections")

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


add_executable(supra
  Squigl.cpp
  fs.cpp
  reflash_flags.cpp
  main.cpp
  seqo.cpp
  melo.cpp
  morsed.cpp
  screen.cpp
  Controls.cpp
  IO.cpp
  jfunc.cpp
  utils.cpp
  clock.cpp
)

add_subdirectory(pico-littlefs/littlefs-lib)
add_subdirectory(pico-littlefs/stdinit-lib)

target_compile_definitions(supra PRIVATE
    PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=0
    PICO_DEFAULT_UART_TX_PIN=12
    PICO_DEFAULT_UART_RX_PIN=13
    PICO_MULTICORE=1
    PICO_FLASH_ASSUME_CORE1_SAFE=1
    PARAM_ASSERTIONS_ENABLED=1 
    PARAM_ASSERTIONS_ENABLED_1=1
    SUPRA_BYPASS_DATE_SET=0
)

target_include_directories(${target_proj} PRIVATE u8g2/csrc)
file(GLOB U8G2_SRC u8g2/csrc/*.c)
add_library(u8g2 ${U8G2_SRC})
target_link_libraries(${target_proj} u8g2)

target_include_directories(supra PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_options(supra PRIVATE -Xlinker --print-memory-usage)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
target_link_libraries(supra 
    pico_stdlib 
    hardware_spi 
    hardware_pwm 
    hardware_pio 
    hardware_i2c 
    littlefs-lib 
    stdinit-lib
    pico_multicore
)

pico_generate_pio_header(supra ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)
# Make sure the build dir is on the include path (usually already is):
target_include_directories(supra PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(supra PRIVATE -w -Werror=all -DCFG_TUSB_DEBUG=1)

target_compile_options(supra PRIVATE -fexceptions)
target_link_options(supra PRIVATE -fexceptions)

pico_enable_stdio_usb(supra 1)
pico_enable_stdio_uart(supra 0)

pico_add_extra_outputs(supra)
