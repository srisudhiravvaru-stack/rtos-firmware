# =============================================================================
# CMakeLists.txt - Industrial RTOS Firmware Project
#
# Supports:
#   - ARM cross-compilation (arm-none-eabi-gcc)
#   - Host-native test builds (Unity unit tests)
#   - Both Release and Debug configurations
#
# Usage:
#   ARM build:
#     cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
#           -DCMAKE_BUILD_TYPE=Release
#
#   Host test build:
#     cmake -B build-test -DBUILD_TESTS=ON -DHOST_BUILD=ON \
#           -DCMAKE_BUILD_TYPE=Debug
# =============================================================================

cmake_minimum_required(VERSION 3.22)

project(rtos_firmware
    VERSION     1.0.0
    DESCRIPTION "Industrial RTOS Firmware"
    LANGUAGES   C ASM
)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(BUILD_TESTS   "Build unit tests for host"     OFF)
option(HOST_BUILD    "Native host build (tests only)" OFF)
option(ENABLE_LTO    "Enable Link-Time Optimization"  OFF)

set(TARGET_MCU "STM32F407VG" CACHE STRING "Target MCU")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Global compile definitions
# ---------------------------------------------------------------------------
add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    USE_HAL_DRIVER
    ${TARGET_MCU}
)

# ---------------------------------------------------------------------------
# Common compile options (applied to all targets)
# ---------------------------------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wshadow
    -Wdouble-promotion
    -Wundef
    -ffunction-sections
    -fdata-sections
    $<$<CONFIG:Debug>:-Og -g3 -ggdb>
    $<$<CONFIG:Release>:-O2>
)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(APP_SOURCES
    src/main.c
    src/task_manager.c
    src/tasks/task_sensor.c
    src/tasks/task_control.c
    src/tasks/task_comms.c
    src/tasks/task_logger.c
    src/tasks/task_watchdog.c
    src/ipc/event_router.c
    src/utils/system_monitor.c
)

set(DRIVER_SOURCES
    drivers/gpio/drv_gpio.c
    drivers/uart/drv_uart.c
    drivers/spi/drv_spi.c
    drivers/i2c/drv_i2c.c
    drivers/timer/drv_timer.c
)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
set(INCLUDE_DIRS
    include
    include/rtos
    include/drivers
    include/middleware
    drivers
    middlewares/freertos/include
    middlewares/freertos/portable/GCC/ARM_CM4F
)

# ---------------------------------------------------------------------------
# FreeRTOS sources (as submodule)
# ---------------------------------------------------------------------------
set(FREERTOS_SOURCES
    middlewares/freertos/tasks.c
    middlewares/freertos/queue.c
    middlewares/freertos/list.c
    middlewares/freertos/timers.c
    middlewares/freertos/event_groups.c
    middlewares/freertos/stream_buffer.c
    middlewares/freertos/croutine.c
    middlewares/freertos/portable/GCC/ARM_CM4F/port.c
    middlewares/freertos/portable/MemMang/heap_4.c
)

# ---------------------------------------------------------------------------
# Firmware executable (ARM cross-compile target)
# ---------------------------------------------------------------------------
if(NOT HOST_BUILD)
    add_executable(${PROJECT_NAME}.elf
        ${APP_SOURCES}
        ${DRIVER_SOURCES}
        ${FREERTOS_SOURCES}
        src/startup_stm32f407xx.s
    )

    target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_DIRS})

    target_link_options(${PROJECT_NAME}.elf PRIVATE
        -T${CMAKE_SOURCE_DIR}/config/STM32F407VGTx_FLASH.ld
        -Wl,--gc-sections
        -Wl,-Map=${PROJECT_NAME}.map
        -Wl,--print-memory-usage
        $<$<BOOL:${ENABLE_LTO}>:-flto>
        --specs=nano.specs
        --specs=nosys.specs
    )

    target_link_libraries(${PROJECT_NAME}.elf PRIVATE m c)

    # Generate .bin and .hex from .elf
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary   $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex     $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_SIZE}    --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
        COMMENT "Generating binary outputs"
    )
endif()

# ---------------------------------------------------------------------------
# Unit Test build (host native)
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
