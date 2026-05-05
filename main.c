/**
 * @file main.c
 * @brief Firmware Entry Point
 *
 * Performs hardware initialization, creates all RTOS IPC objects and tasks,
 * then starts the FreeRTOS scheduler. Nothing after vTaskStartScheduler()
 * should ever execute.
 *
 * @copyright Copyright (c) 2025. MIT License.
 */

#include "task_manager.h"
#include "FreeRTOS.h"
#include "task.h"

/* HAL/BSP includes - replace with your target's HAL */
/* #include "stm32f4xx_hal.h" */
/* #include "bsp.h"           */

/*---------------------------------------------------------------------------
 * Private function prototypes
 *---------------------------------------------------------------------------*/
static void SystemClock_Config( void );
static void Hardware_Init( void );

/*---------------------------------------------------------------------------
 * Entry Point
 *---------------------------------------------------------------------------*/
int main( void )
{
    /* 1. Configure system clock (before any peripheral init) */
    SystemClock_Config();

    /* 2. Initialize hardware peripherals (GPIO, UART, SPI, I2C, Watchdog) */
    Hardware_Init();

    /* 3. Create all IPC primitives (queues, mutexes, event groups)
     *    Must be done BEFORE creating tasks that reference them.          */
    if( TaskManager_InitIPC() != pdTRUE )
    {
        /* Fatal: cannot proceed without IPC - trigger hardware reset */
        /* HAL_NVIC_SystemReset(); */
        for( ;; ) {}
    }

    /* 4. Create all application tasks */
    if( TaskManager_CreateTasks() != pdTRUE )
    {
        /* Fatal: task creation failed (likely out of heap) */
        for( ;; ) {}
    }

    /* 5. Start the FreeRTOS scheduler - this call never returns */
    vTaskStartScheduler();

    /* Should never reach here. If it does, heap is exhausted. */
    for( ;; ) {}

    return 0; /* Suppress compiler warning */
}

/*---------------------------------------------------------------------------
 * System Clock Configuration
 * Target: STM32F407 @ 168 MHz using HSE + PLL
 *---------------------------------------------------------------------------*/
static void SystemClock_Config( void )
{
    /* TODO: Configure HSE -> PLL -> SYSCLK @ 168 MHz
     *       AHB  = 168 MHz (HPRE  = /1)
     *       APB1 = 42 MHz  (PPRE1 = /4)
     *       APB2 = 84 MHz  (PPRE2 = /2)
     *
     * Replace with HAL_RCC_OscConfig() + HAL_RCC_ClockConfig() calls.
     */
}

/*---------------------------------------------------------------------------
 * Hardware Peripheral Initialization
 *---------------------------------------------------------------------------*/
static void Hardware_Init( void )
{
    /* TODO: Initialize in this order to satisfy dependencies:
     *
     *  1. HAL_Init()             - SysTick, HAL tick base
     *  2. GPIO_Init()            - Status LEDs, button inputs
     *  3. UART_Init()            - Debug console / field bus
     *  4. I2C_Init()             - Sensor bus
     *  5. SPI_Init()             - External flash, radio
     *  6. Timer_Init()           - PWM outputs, runtime stats counter
     *  7. Watchdog_Init()        - IWDG with calculated timeout
     *  8. BSP_Console_Init()     - Redirect printf to UART
     */
}
