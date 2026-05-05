/**
 * @file task_sensor.c
 * @brief Sensor Acquisition Task
 *
 * Reads sensor data at a fixed rate using vTaskDelayUntil() for
 * precise, jitter-resistant periodic execution. Posts data to
 * g_qSensorData for the control task to consume.
 *
 * @copyright Copyright (c) 2025. MIT License.
 */

#include "tasks/task_sensor.h"
#include "task_manager.h"
#include "drivers/i2c.h"
#include "drivers/spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include <string.h>

/*---------------------------------------------------------------------------
 * Private Constants
 *---------------------------------------------------------------------------*/
#define SENSOR_SAMPLE_PERIOD_MS     ( 10U )     /* 100 Hz sample rate        */
#define SENSOR_QUEUE_TIMEOUT_MS     ( 5U )      /* Max wait to post to queue */
#define SENSOR_MODULE_ID            ( 0x01U )

/*---------------------------------------------------------------------------
 * Private Types
 *---------------------------------------------------------------------------*/
typedef struct {
    bool     initialized;
    uint32_t sample_count;
    uint32_t error_count;
} SensorTaskState_t;

/*---------------------------------------------------------------------------
 * Private Variables
 *---------------------------------------------------------------------------*/
static SensorTaskState_t s_state = { 0 };

/*---------------------------------------------------------------------------
 * Private Function Prototypes
 *---------------------------------------------------------------------------*/
static bool  SensorTask_Init( void );
static bool  SensorTask_Read( SensorDataMsg_t *pMsg );
static void  SensorTask_HandleError( void );

/*---------------------------------------------------------------------------
 * Task Entry Point
 *---------------------------------------------------------------------------*/
void vTaskSensor( void *pvParameters )
{
    ( void ) pvParameters;

    TickType_t       xLastWakeTime;
    SensorDataMsg_t  sensorMsg;
    bool             readOk;

    TaskManager_Log( LOG_INFO, SENSOR_MODULE_ID, "Sensor task starting" );

    /* --- Initialization --- */
    if( !SensorTask_Init() )
    {
        TaskManager_Log( LOG_CRITICAL, SENSOR_MODULE_ID,
                         "Sensor init FAILED - suspending task" );
        vTaskSuspend( NULL );
    }

    /* Signal system that sensors are ready */
    xEventGroupSetBits( g_evSystemFlags, EVT_SENSOR_READY );

    /* --- Wait for all subsystems before starting acquisition --- */
    xEventGroupWaitBits( g_evSystemFlags,
                         EVT_ALL_INIT_DONE,
                         pdFALSE,   /* Don't clear bits */
                         pdTRUE,    /* Wait for ALL bits */
                         pdMS_TO_TICKS( 5000U ) );

    /* Capture the current time for vTaskDelayUntil */
    xLastWakeTime = xTaskGetTickCount();

    /* --- Main Task Loop --- */
    for( ;; )
    {
        /* Block until next sample period - precise periodic execution */
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( SENSOR_SAMPLE_PERIOD_MS ) );

        /* Read all sensor channels */
        readOk = SensorTask_Read( &sensorMsg );

        if( readOk )
        {
            s_state.sample_count++;
            sensorMsg.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

            /* Post to control task queue - overwrite oldest if full */
            if( xQueueSend( g_qSensorData,
                            &sensorMsg,
                            pdMS_TO_TICKS( SENSOR_QUEUE_TIMEOUT_MS ) ) != pdTRUE )
            {
                /* Queue full: control task can't keep up */
                TaskManager_Log( LOG_WARN, SENSOR_MODULE_ID,
                                 "Sensor queue full - sample dropped (cnt=%lu)",
                                 s_state.sample_count );
            }
        }
        else
        {
            SensorTask_HandleError();
        }

        /* Stack high-water mark check (debug builds only) */
#ifdef DEBUG
        UBaseType_t hwm = uxTaskGetStackHighWaterMark( NULL );
        if( hwm < 32U )
        {
            TaskManager_Log( LOG_WARN, SENSOR_MODULE_ID,
                             "Sensor task stack low! HWM=%u words", hwm );
        }
#endif
    }
}

/*---------------------------------------------------------------------------
 * Private: Sensor Initialization
 *---------------------------------------------------------------------------*/
static bool SensorTask_Init( void )
{
    /* Acquire I2C bus mutex */
    if( xSemaphoreTake( g_mutexI2C, pdMS_TO_TICKS( 100U ) ) != pdTRUE )
    {
        return false;
    }

    /* TODO: Initialize I2C sensor (e.g., BME280 at address 0x76) */
    /* bool ok = drv_i2c_write_reg( 0x76, REG_RESET, 0xB6 ); */

    xSemaphoreGive( g_mutexI2C );

    s_state.initialized = true;
    s_state.error_count = 0U;

    return true;
}

/*---------------------------------------------------------------------------
 * Private: Read Sensor Data
 *---------------------------------------------------------------------------*/
static bool SensorTask_Read( SensorDataMsg_t *pMsg )
{
    configASSERT( pMsg != NULL );

    memset( pMsg, 0, sizeof( *pMsg ) );
    pMsg->sensor_id = SENSOR_MODULE_ID;

    /* Acquire I2C bus */
    if( xSemaphoreTake( g_mutexI2C, pdMS_TO_TICKS( 5U ) ) != pdTRUE )
    {
        return false;
    }

    /* TODO: Read sensor registers and populate pMsg fields
     *
     * Example (BME280):
     *   uint8_t buf[6];
     *   drv_i2c_read( 0x76, REG_PRESS_MSB, buf, 6 );
     *   pMsg->pressure    = bme280_compensate_pressure( buf );
     *   pMsg->temperature = bme280_compensate_temp( buf + 3 );
     */

    /* Placeholder values for skeleton */
    pMsg->temperature = 25.0f;
    pMsg->pressure    = 101325.0f;
    pMsg->status      = 0x01U;  /* STATUS_OK */

    xSemaphoreGive( g_mutexI2C );
    return true;
}

/*---------------------------------------------------------------------------
 * Private: Error Handler
 *---------------------------------------------------------------------------*/
static void SensorTask_HandleError( void )
{
    s_state.error_count++;

    TaskManager_Log( LOG_ERROR, SENSOR_MODULE_ID,
                     "Sensor read failed (err_count=%lu)", s_state.error_count );

    if( s_state.error_count > 10U )
    {
        /* Too many consecutive errors: set fault flag */
        xEventGroupSetBits( g_evSystemFlags, EVT_FAULT_DETECTED );
        TaskManager_Log( LOG_CRITICAL, SENSOR_MODULE_ID,
                         "Sensor fault declared after %lu errors",
                         s_state.error_count );
        /* Attempt reinitialization */
        s_state.initialized = false;
        SensorTask_Init();
        s_state.error_count = 0U;
    }
}
