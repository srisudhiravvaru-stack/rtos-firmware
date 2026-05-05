/**
 * @file task_manager.h
 * @brief Task Management and IPC Interface
 *
 * Central registry for all application tasks, priorities, stack sizes,
 * and inter-task communication handles (queues, mutexes, semaphores).
 *
 * @copyright Copyright (c) 2025. MIT License.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "message_buffer.h"
#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------
 * Task Priority Definitions (higher number = higher priority)
 * Reserve configMAX_PRIORITIES-1 for the Timer daemon task.
 *---------------------------------------------------------------------------*/
typedef enum {
    TASK_PRIORITY_IDLE       = tskIDLE_PRIORITY,     /* 0 - FreeRTOS idle   */
    TASK_PRIORITY_LOW        = 1,
    TASK_PRIORITY_BELOW_NRM  = 2,
    TASK_PRIORITY_NORMAL     = 3,
    TASK_PRIORITY_ABOVE_NRM  = 4,
    TASK_PRIORITY_HIGH       = 5,
    TASK_PRIORITY_REALTIME   = 6,
    TASK_PRIORITY_CRITICAL   = 7,
} TaskPriority_t;

/*---------------------------------------------------------------------------
 * Stack Sizes (in words, not bytes)
 *---------------------------------------------------------------------------*/
#define STACK_SIZE_TINY         ( 128U )
#define STACK_SIZE_SMALL        ( 256U )
#define STACK_SIZE_MEDIUM       ( 512U )
#define STACK_SIZE_LARGE        ( 1024U )
#define STACK_SIZE_XLARGE       ( 2048U )

/*---------------------------------------------------------------------------
 * Application Task Handles
 *---------------------------------------------------------------------------*/
extern TaskHandle_t g_hTaskSensor;
extern TaskHandle_t g_hTaskControl;
extern TaskHandle_t g_hTaskComms;
extern TaskHandle_t g_hTaskLogger;
extern TaskHandle_t g_hTaskWatchdog;

/*---------------------------------------------------------------------------
 * IPC: Message Queue Handles
 *---------------------------------------------------------------------------*/
extern QueueHandle_t g_qSensorData;     /* Sensor -> Control                */
extern QueueHandle_t g_qCommandQueue;   /* Comms  -> Control                */
extern QueueHandle_t g_qLogQueue;       /* All    -> Logger                 */

/*---------------------------------------------------------------------------
 * IPC: Mutex Handles (shared resource protection)
 *---------------------------------------------------------------------------*/
extern SemaphoreHandle_t g_mutexSPI;    /* SPI bus arbitration              */
extern SemaphoreHandle_t g_mutexI2C;    /* I2C bus arbitration              */
extern SemaphoreHandle_t g_mutexFlash;  /* Flash write protection           */

/*---------------------------------------------------------------------------
 * IPC: Event Group Bits (system-wide state flags)
 *---------------------------------------------------------------------------*/
extern EventGroupHandle_t g_evSystemFlags;

#define EVT_SENSOR_READY        ( 1UL << 0 )
#define EVT_COMMS_CONNECTED     ( 1UL << 1 )
#define EVT_FAULT_DETECTED      ( 1UL << 2 )
#define EVT_CALIBRATION_DONE    ( 1UL << 3 )
#define EVT_SHUTDOWN_REQUEST    ( 1UL << 4 )
#define EVT_CONFIG_UPDATED      ( 1UL << 5 )
#define EVT_ALL_INIT_DONE       ( EVT_SENSOR_READY | EVT_COMMS_CONNECTED )

/*---------------------------------------------------------------------------
 * IPC: Message Structures
 *---------------------------------------------------------------------------*/

/** Sensor data payload sent over g_qSensorData */
typedef struct {
    uint32_t timestamp_ms;      /**< System tick timestamp (ms)             */
    float    temperature;       /**< Temperature reading (°C)               */
    float    pressure;          /**< Pressure reading (Pa)                  */
    uint16_t raw_adc[4];        /**< Raw ADC channel values                 */
    uint8_t  sensor_id;         /**< Source sensor identifier               */
    uint8_t  status;            /**< Sensor status flags                    */
} SensorDataMsg_t;

/** Command structure sent from comms to control */
typedef struct {
    uint8_t  cmd_id;            /**< Command identifier                     */
    uint8_t  source;            /**< Origin: UART, MQTT, CAN, etc.          */
    uint16_t length;            /**< Payload length in bytes                */
    uint8_t  payload[32];       /**< Command payload data                   */
} CommandMsg_t;

/** Log entry queued for the logger task */
typedef struct {
    uint32_t timestamp_ms;
    uint8_t  level;             /**< LOG_DEBUG / INFO / WARN / ERROR / CRIT */
    uint8_t  module_id;
    char     message[80];
} LogMsg_t;

/*---------------------------------------------------------------------------
 * Log Levels
 *---------------------------------------------------------------------------*/
typedef enum {
    LOG_DEBUG    = 0,
    LOG_INFO     = 1,
    LOG_WARN     = 2,
    LOG_ERROR    = 3,
    LOG_CRITICAL = 4,
} LogLevel_t;

/*---------------------------------------------------------------------------
 * Function Prototypes
 *---------------------------------------------------------------------------*/

/**
 * @brief Initialize all IPC primitives (queues, mutexes, event groups).
 *        Must be called before the scheduler starts.
 * @return pdTRUE on success, pdFALSE on allocation failure.
 */
BaseType_t TaskManager_InitIPC( void );

/**
 * @brief Create and register all application tasks.
 *        Must be called after TaskManager_InitIPC().
 * @return pdTRUE on success, pdFALSE if any task creation fails.
 */
BaseType_t TaskManager_CreateTasks( void );

/**
 * @brief Post a log message from any context (task or ISR safe via queue).
 * @param level   Severity level.
 * @param module  Numeric module/subsystem ID.
 * @param fmt     printf-style format string (truncated to LogMsg_t size).
 */
void TaskManager_Log( LogLevel_t level, uint8_t module, const char *fmt, ... );

#ifdef __cplusplus
}
#endif

#endif /* TASK_MANAGER_H */
