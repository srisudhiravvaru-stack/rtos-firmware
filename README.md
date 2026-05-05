# Industrial RTOS Firmware

[![CI](https://github.com/your-org/rtos-firmware/actions/workflows/ci.yml/badge.svg)](https://github.com/your-org/rtos-firmware/actions)
[![Docs](https://img.shields.io/badge/docs-Doxygen-blue)](https://your-org.github.io/rtos-firmware)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C Standard](https://img.shields.io/badge/C-C11-blue)](CMakeLists.txt)

Production-grade embedded firmware for ARM Cortex-M4 microcontrollers using FreeRTOS. Designed for industrial sensor acquisition, real-time control loops, and reliable field communication — with safety-critical best practices throughout.

---

## System Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                       │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────┐  ┌────────┐  │
│  │ Sensor Task │  │Control Task │  │Comms Task│  │ Logger │  │
│  │ (RT, 100Hz) │  │ (High, PID) │  │ (Normal) │  │ (Low)  │  │
│  └──────┬──────┘  └──────┬──────┘  └────┬─────┘  └───┬────┘  │
│         │   SensorDataQ  │  CommandQ    │       LogQ  │       │
│  ┌──────▼──────────────────────────────────────────────────┐  │
│  │              IPC LAYER  (Queues / Mutexes / Events)     │  │
│  └──────────────────────┬──────────────────────────────────┘  │
│  ┌───────────────────────▼────────────────────────────────┐   │
│  │                   FreeRTOS Kernel                      │   │
│  │         (Static allocation, Stack overflow check)      │   │
│  └───────────────────────┬────────────────────────────────┘   │
└──────────────────────────┼─────────────────────────────────────┘
                           │
┌──────────────────────────▼─────────────────────────────────────┐
│                       DRIVER LAYER                              │
│   GPIO │ UART │ SPI │ I2C │ Timer │ ADC │ PWM │ Watchdog       │
└─────────────────────────────────────────────────────────────────┘
```

### Task Overview

| Task | Priority | Period | Stack | Description |
|---|---|---|---|---|
| `Watchdog` | CRITICAL (7) | Event | 256W | Hardware watchdog feeder, fault monitor |
| `Sensor` | REALTIME (6) | 10ms | 512W | Reads I2C/SPI sensors at 100Hz |
| `Control` | HIGH (5) | 10ms | 512W | PID control loop, actuator output |
| `Comms` | NORMAL (3) | Event | 1024W | UART/MQTT command handling |
| `Logger` | LOW (1) | Event | 1024W | Deferred log flushing to storage/UART |

### IPC Channels

| Handle | Type | Producer | Consumer | Depth |
|---|---|---|---|---|
| `g_qSensorData` | Queue (static) | Sensor | Control | 10 msgs |
| `g_qCommandQueue` | Queue | Comms | Control | 8 msgs |
| `g_qLogQueue` | Queue (static) | All | Logger | 32 msgs |
| `g_mutexSPI` | Mutex | — | Sensor, Comms | — |
| `g_mutexI2C` | Mutex | — | Sensor | — |
| `g_evSystemFlags` | EventGroup | All | All | 8 bits |

---

## Hardware Requirements

### Reference Platform: STM32F407VG Discovery

| Peripheral | Interface | Usage |
|---|---|---|
| Cortex-M4 @ 168 MHz | — | Main processor |
| FPU (fpv4-sp-d16) | — | Hardware float for control |
| BME280 | I2C1 (0x76) | Temperature / pressure sensor |
| External Flash | SPI1 | Log storage |
| RS-485 / UART2 | UART | Field bus command interface |
| PA5 (LD2) | GPIO | System heartbeat LED |
| IWDG | Internal | Independent hardware watchdog |

> **Porting to another MCU:** Update `cmake/arm-none-eabi.cmake` (`-mcpu`, `-mfpu`), replace `startup_stm32f407xx.s` and the linker script, and update the driver HAL includes.

---

## Getting Started

### Prerequisites

| Tool | Version | Install |
|---|---|---|
| arm-none-eabi-gcc | ≥ 13.0 | `sudo apt install gcc-arm-none-eabi` |
| CMake | ≥ 3.22 | `sudo apt install cmake` |
| Ninja | ≥ 1.11 | `sudo apt install ninja-build` |
| OpenOCD | ≥ 0.12 | `sudo apt install openocd` |
| Doxygen | ≥ 1.9 | `sudo apt install doxygen graphviz` |

### 1. Clone with Submodules

```bash
git clone --recurse-submodules https://github.com/your-org/rtos-firmware.git
cd rtos-firmware
```

### 2. Build (ARM Release)

```bash
cmake -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTARGET_MCU=STM32F407VG

cmake --build build --parallel
```

Output: `build/rtos_firmware.elf`, `.bin`, `.hex`, `.map`

### 3. Flash to Target

```bash
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program build/rtos_firmware.elf verify reset exit"
```

### 4. Run Unit Tests (Host)

```bash
cmake -B build-test \
  -DBUILD_TESTS=ON \
  -DHOST_BUILD=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build-test --parallel
cd build-test && ctest --output-on-failure
```

### 5. Generate Documentation

```bash
doxygen docs/doxygen/Doxyfile
# Open docs/html/index.html
```

### 6. Check Code Formatting

```bash
# Check only (CI mode)
find src include drivers -name '*.[ch]' | xargs clang-format --dry-run --Werror

# Auto-fix in place
find src include drivers -name '*.[ch]' | xargs clang-format -i
```

---

## Project Structure

```
rtos-firmware/
├── .github/
│   └── workflows/
│       └── ci.yml              # CI/CD pipeline (lint, build, test, docs)
├── cmake/
│   └── arm-none-eabi.cmake     # ARM cross-compilation toolchain file
├── config/
│   └── STM32F407VGTx_FLASH.ld  # Linker script
├── docs/
│   ├── doxygen/
│   │   └── Doxyfile
│   └── diagrams/               # Architecture diagrams (PlantUML)
├── drivers/
│   ├── gpio/                   # GPIO HAL abstraction
│   ├── uart/                   # UART driver
│   ├── spi/                    # SPI bus driver
│   ├── i2c/                    # I2C bus driver
│   └── timer/                  # Hardware timer driver
├── include/
│   ├── task_manager.h          # Task handles, IPC handles, message types
│   ├── drivers/                # Driver public APIs
│   ├── middleware/             # Middleware public APIs
│   └── rtos/
│       └── FreeRTOSConfig.h    # RTOS configuration
├── middlewares/
│   ├── freertos/               # FreeRTOS kernel (git submodule)
│   └── mqtt/                   # MQTT client (git submodule)
├── scripts/
│   ├── flash.sh                # OpenOCD flash helper
│   └── run_tests.sh            # Test runner with coverage
├── src/
│   ├── main.c                  # Entry point, system init, scheduler start
│   ├── task_manager.c          # Task creation, IPC init, RTOS hooks
│   ├── tasks/
│   │   ├── task_sensor.c       # Sensor acquisition task
│   │   ├── task_control.c      # PID control loop task
│   │   ├── task_comms.c        # Communications task
│   │   ├── task_logger.c       # Deferred logger task
│   │   └── task_watchdog.c     # Watchdog feeder task
│   ├── ipc/
│   │   └── event_router.c      # System event dispatch helpers
│   └── utils/
│       └── system_monitor.c    # Runtime stats, stack HWM reporter
├── tests/
│   ├── CMakeLists.txt
│   ├── unity/                  # Unity test framework (submodule)
│   ├── mocks/                  # CMock stubs for FreeRTOS APIs
│   ├── unit/
│   │   ├── test_pid.c
│   │   ├── test_sensor_queue.c
│   │   └── test_driver_uart.c
│   └── integration/
│       └── test_sensor_control_pipeline.c
├── .clang-format               # Code style rules (Allman, 4-space indent)
├── .gitmodules                 # Submodule references
├── CMakeLists.txt              # Top-level build definition
├── CONTRIBUTING.md
├── LICENSE
├── README.md
└── SECURITY.md
```

---

## Configuration

Key parameters to adjust in `include/rtos/FreeRTOSConfig.h`:

| Macro | Default | Description |
|---|---|---|
| `configCPU_CLOCK_HZ` | 168000000 | MCU core clock frequency |
| `configTICK_RATE_HZ` | 1000 | RTOS tick rate (1ms) |
| `configTOTAL_HEAP_SIZE` | 65536 (64KB) | FreeRTOS heap size |
| `configMAX_PRIORITIES` | 10 | Maximum task priority levels |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 | Stack paint + check method |

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for branch strategy, commit conventions, and the code review checklist.

## Security

See [SECURITY.md](SECURITY.md) for vulnerability reporting policy.

## License

[MIT License](LICENSE) — Copyright (c) 2025
