/**
 * @file task_manager.c
 * @brief Task Manager Implementation
 *
 * Creates all RTOS tasks and IPC objects. All handles are stored in globals
 * so any module can reference them. Static allocation is used for
 * safety-critical tasks to guarantee memory at link time.
 *
 * @copyright Copyright (c) 2025. MIT License.
 */

#include "task_manager.h"
#include "tasks/task_sensor.h"
#include "tasks/task_control.h"
#include "tasks/task_comms.h"
#include "tasks/task_logger.h"
#include "tasks/task_watchdog.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*---------------------------------------------------------------------------
 * Task Handles (global)
 *---------------------------------------------------------------------------*/
TaskHandle_t g_hTaskSensor   = NULL;
TaskHandle_t g_hTaskControl  = NULL;
TaskHandle_t g_hTaskComms    = NULL;
TaskHandle_t g_hTaskLogger   = NULL;
TaskHandle_t g_hTaskWatchdog = NULL;

/*---------------------------------------------------------------------------
 * IPC Queue Handles (global)
 *---------------------------------------------------------------------------*/
QueueHandle_t g_qSensorData    = NULL;
QueueHandle_t g_qCommandQueue  = NULL;
QueueHandle_t g_qLogQueue      = NULL;

/*---------------------------------------------------------------------------
 * Mutex Handles (global)
 *---------------------------------------------------------------------------*/
SemaphoreHandle_t g_mutexSPI   = NULL;
SemaphoreHandle_t g_mutexI2C   = NULL;
SemaphoreHandle_t g_mutexFlash = NULL;

/*---------------------------------------------------------------------------
 * Event Group Handle (global)
 *---------------------------------------------------------------------------*/
EventGroupHandle_t g_evSystemFlags = NULL;

/*---------------------------------------------------------------------------
 * Static Allocation Buffers (for safety-critical tasks)
 * Using static allocation avoids heap fragmentation and guarantees
 * memory availability at compile/link time.
 *---------------------------------------------------------------------------*/
static StaticTask_t  s_taskBufSensor;
static StackType_t   s_stackSensor[STACK_SIZE_MEDIUM];

static StaticTask_t  s_taskBufControl;
static StackType_t   s_stackControl[STACK_SIZE_MEDIUM];

static StaticTask_t  s_taskBufWatchdog;
static StackType_t   s_stackWatchdog[STACK_SIZE_SMALL];

/* Static queue storage */
static StaticQueue_t    s_queueBufSensor;
static uint8_t          s_queueStorageSensor[10 * sizeof(SensorDataMsg_t)];

static StaticQueue_t    s_queueBufLog;
static uint8_t          s_queueStorageLog[32 * sizeof(LogMsg_t)];

/*---------------------------------------------------------------------------
 * Public: Initialize IPC Primitives
 *---------------------------------------------------------------------------*/
BaseType_t TaskManager_InitIPC( void )
{
    /* --- Queues --- */
    /* Sensor queue: statically allocated for deterministic behavior */
    g_qSensorData = xQueueCreateStatic( 10,
                                        sizeof( SensorDataMsg_t ),
                                        s_queueStorageSensor,
                                        &s_queueBufSensor );

    /* Command queue: dynamically allocated (non-critical path) */
    g_qCommandQueue = xQueueCreate( 8, sizeof( CommandMsg_t ) );

    /* Log queue: statically allocated, oversized to avoid drops */
    g_qLogQueue = xQueueCreateStatic( 32,
                                      sizeof( LogMsg_t ),
                                      s_queueStorageLog,
                                      &s_queueBufLog );

    /* --- Mutexes --- */
    g_mutexSPI   = xSemaphoreCreateMutex();
    g_mutexI2C   = xSemaphoreCreateMutex();
    g_mutexFlash = xSemaphoreCreateMutex();

    /* --- Event Groups --- */
    g_evSystemFlags = xEventGroupCreate();

    /* --- Validate all handles --- */
    if( ( g_qSensorData    == NULL ) ||
        ( g_qCommandQueue  == NULL ) ||
        ( g_qLogQueue      == NULL ) ||
        ( g_mutexSPI       == NULL ) ||
        ( g_mutexI2C       == NULL ) ||
        ( g_mutexFlash     == NULL ) ||
        ( g_evSystemFlags  == NULL ) )
    {
        return pdFALSE;
    }

    /* Assign debug-friendly names visible in trace tools (e.g., Tracealyzer) */
    vQueueAddToRegistry( g_qSensorData,   "SensorDataQ" );
    vQueueAddToRegistry( g_qCommandQueue, "CommandQ"    );
    vQueueAddToRegistry( g_qLogQueue,     "LogQ"        );

    return pdTRUE;
}

/*---------------------------------------------------------------------------
 * Public: Create Application Tasks
 *---------------------------------------------------------------------------*/
BaseType_t TaskManager_CreateTasks( void )
{
    BaseType_t xResult;

    /* Watchdog task: static allocation, highest application priority */
    g_hTaskWatchdog = xTaskCreateStatic( vTaskWatchdog,
                                         "Watchdog",
                                         STACK_SIZE_SMALL,
                                         NULL,
                                         TASK_PRIORITY_CRITICAL,
                                         s_stackWatchdog,
                                         &s_taskBufWatchdog );

    /* Sensor task: static allocation, real-time priority */
    g_hTaskSensor = xTaskCreateStatic( vTaskSensor,
                                       "Sensor",
                                       STACK_SIZE_MEDIUM,
                                       NULL,
                                       TASK_PRIORITY_REALTIME,
                                       s_stackSensor,
                                       &s_taskBufSensor );

    /* Control task: static allocation, high priority */
    g_hTaskControl = xTaskCreateStatic( vTaskControl,
                                        "Control",
                                        STACK_SIZE_MEDIUM,
                                        NULL,
                                        TASK_PRIORITY_HIGH,
                                        s_stackControl,
                                        &s_taskBufControl );

    /* Comms task: dynamic allocation acceptable for non-safety path */
    xResult = xTaskCreate( vTaskComms,
                           "Comms",
                           STACK_SIZE_LARGE,
                           NULL,
                           TASK_PRIORITY_NORMAL,
                           &g_hTaskComms );

    if( xResult != pdPASS ) { return pdFALSE; }

    /* Logger task: lowest non-idle priority, large stack for printf */
    xResult = xTaskCreate( vTaskLogger,
                           "Logger",
                           STACK_SIZE_LARGE,
                           NULL,
                           TASK_PRIORITY_LOW,
                           &g_hTaskLogger );

    if( xResult != pdPASS ) { return pdFALSE; }

    /* Verify static task creation */
    if( ( g_hTaskWatchdog == NULL ) ||
        ( g_hTaskSensor   == NULL ) ||
        ( g_hTaskControl  == NULL ) )
    {
        return pdFALSE;
    }

    return pdTRUE;
}

/*---------------------------------------------------------------------------
 * Public: Thread-safe logging helper
 *---------------------------------------------------------------------------*/
void TaskManager_Log( LogLevel_t level, uint8_t module, const char *fmt, ... )
{
    LogMsg_t msg;
    va_list  args;

    msg.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    msg.level        = (uint8_t)level;
    msg.module_id    = module;

    va_start( args, fmt );
    vsnprintf( msg.message, sizeof( msg.message ), fmt, args );
    va_end( args );

    /* Non-blocking post: drop log if queue is full rather than stalling caller */
    xQueueSend( g_qLogQueue, &msg, 0 );
}

/*---------------------------------------------------------------------------
 * FreeRTOS Hook Functions
 *---------------------------------------------------------------------------*/

/**
 * @brief Stack overflow detection hook.
 *        In production: trigger a system fault / safe-state handler.
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    ( void ) xTask;
    ( void ) pcTaskName;
    /* TODO: Log fault, enter safe state, trigger watchdog reset */
    taskDISABLE_INTERRUPTS();
    for( ;; ) { /* Halt - allow watchdog to reset system */ }
}

/**
 * @brief Malloc failure hook.
 */
void vApplicationMallocFailedHook( void )
{
    /* TODO: Log fault, enter safe state */
    taskDISABLE_INTERRUPTS();
    for( ;; ) {}
}

/**
 * @brief Idle hook - put CPU into low-power sleep when idle.
 */
void vApplicationIdleHook( void )
{
    /* __WFI() on ARM Cortex-M: wait for interrupt, saves power */
    /* __WFI(); */
}

/**
 * @brief Tick hook - called from tick ISR every configTICK_RATE_HZ.
 */
void vApplicationTickHook( void )
{
    /* Optional: Feed a secondary software watchdog counter here */
}

/**
 * @brief Assert handler - logs file/line and halts for debugging.
 */
void vAssertCalled( const char *pcFile, uint32_t ulLine )
{
    taskDISABLE_INTERRUPTS();
    /* In debug builds: breakpoint here. In release: trigger safe-state. */
    ( void ) pcFile;
    ( void ) ulLine;
    for( ;; ) {}
}

/*---------------------------------------------------------------------------
 * Static allocation support stubs (required when
 * configSUPPORT_STATIC_ALLOCATION == 1)
 *---------------------------------------------------------------------------*/
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t      *pulIdleTaskStackSize )
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t  **ppxTimerTaskStackBuffer,
                                     uint32_t      *pulTimerTaskStackSize )
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
