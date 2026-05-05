# =============================================================================
# cmake/arm-none-eabi.cmake
# CMake toolchain file for ARM Cortex-M cross-compilation
# =============================================================================

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain prefix - override with ARM_TOOLCHAIN_PATH env var if needed
set(TOOLCHAIN_PREFIX "arm-none-eabi-")

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc  REQUIRED)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc  REQUIRED)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size    REQUIRED)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# CPU and FPU flags for Cortex-M4 with hardware FPU
# Change -mcpu and -mfpu for other targets (e.g., M0, M33)
# ---------------------------------------------------------------------------
set(CPU_FLAGS
    "-mcpu=cortex-m4 \
     -mthumb \
     -mfpu=fpv4-sp-d16 \
     -mfloat-abi=hard"
)

set(CMAKE_C_FLAGS_INIT   "${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS}")

# Sysroot search (cross-compiled libraries)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
