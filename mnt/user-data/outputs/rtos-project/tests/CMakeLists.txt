# =============================================================================
# tests/CMakeLists.txt
# Unity-based unit test suite for host-native build
# =============================================================================

cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Unity test framework (fetched or as submodule at tests/unity)
# ---------------------------------------------------------------------------
add_library(unity STATIC
    unity/src/unity.c
)
target_include_directories(unity PUBLIC unity/src)

# ---------------------------------------------------------------------------
# CMock (optional mock framework)
# ---------------------------------------------------------------------------
add_library(cmock STATIC
    unity/extras/CMock/src/cmock.c
)
target_include_directories(cmock PUBLIC unity/extras/CMock/src)
target_link_libraries(cmock PUBLIC unity)

# ---------------------------------------------------------------------------
# Helper macro: add_unity_test(NAME sources...)
# Creates an executable, links Unity, registers with ctest.
# ---------------------------------------------------------------------------
macro(add_unity_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/rtos
        ${CMAKE_SOURCE_DIR}/tests/mocks
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${TEST_NAME} PRIVATE unity cmock m)
    target_compile_definitions(${TEST_NAME} PRIVATE HOST_BUILD=1 UNIT_TEST=1)

    # Suppress unused-parameter warnings in test files
    target_compile_options(${TEST_NAME} PRIVATE -Wno-unused-parameter)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# ---------------------------------------------------------------------------
# Unit test targets
# ---------------------------------------------------------------------------
add_unity_test(test_pid
    unit/test_pid.c
    ${CMAKE_SOURCE_DIR}/src/tasks/task_control.c
)

add_unity_test(test_sensor_queue
    unit/test_sensor_queue.c
    mocks/mock_freertos_queue.c
)

add_unity_test(test_task_manager_ipc
    unit/test_task_manager_ipc.c
    mocks/mock_freertos_queue.c
    mocks/mock_freertos_semphr.c
)

add_unity_test(test_driver_uart
    unit/test_driver_uart.c
    ${CMAKE_SOURCE_DIR}/drivers/uart/drv_uart.c
    mocks/mock_hal_uart.c
)

# ---------------------------------------------------------------------------
# Integration test targets
# ---------------------------------------------------------------------------
add_unity_test(test_sensor_to_control_pipeline
    integration/test_sensor_control_pipeline.c
    ${CMAKE_SOURCE_DIR}/src/tasks/task_sensor.c
    ${CMAKE_SOURCE_DIR}/src/tasks/task_control.c
    mocks/mock_freertos_queue.c
)
