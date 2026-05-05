/**
 * @file task_control.c
 * @brief Control Loop Task
 *
 * Consumes sensor data from g_qSensorData, applies control algorithms,
 * and drives actuator outputs. Also processes commands from g_qCommandQueue.
 *
 * @copyright Copyright (c) 2025. MIT License.
 */

#include "tasks/task_control.h"
#include "task_manager.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>
#include <math.h>

#define CONTROL_MODULE_ID    ( 0x02U )
#define CONTROL_LOOP_MS      ( 10U )

/*---------------------------------------------------------------------------
 * Private: PID State (example control structure)
 *---------------------------------------------------------------------------*/
typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
    float setpoint;
    float output_min;
    float output_max;
} PID_t;

static PID_t s_pid = {
    .kp = 1.0f, .ki = 0.1f, .kd = 0.05f,
    .output_min = -100.0f, .output_max = 100.0f,
    .setpoint = 25.0f
};

/*---------------------------------------------------------------------------
 * Private: PID Update
 *---------------------------------------------------------------------------*/
static float PID_Update( PID_t *pid, float measurement, float dt_s )
{
    float error    = pid->setpoint - measurement;
    pid->integral += error * dt_s;

    /* Anti-windup: clamp integral */
    if( pid->integral > pid->output_max ) pid->integral = pid->output_max;
    if( pid->integral < pid->output_min ) pid->integral = pid->output_min;

    float derivative = ( error - pid->prev_error ) / dt_s;
    pid->prev_error  = error;

    float output = ( pid->kp * error ) +
                   ( pid->ki * pid->integral ) +
                   ( pid->kd * derivative );

    /* Clamp output */
    if( output > pid->output_max ) output = pid->output_max;
    if( output < pid->output_min ) output = pid->output_min;

    return output;
}

/*---------------------------------------------------------------------------
 * Task Entry Point
 *---------------------------------------------------------------------------*/
void vTaskControl( void *pvParameters )
{
    ( void ) pvParameters;

    SensorDataMsg_t  sensorMsg;
    CommandMsg_t     cmdMsg;
    float            control_output;
    const float      dt_s = CONTROL_LOOP_MS / 1000.0f;

    TaskManager_Log( LOG_INFO, CONTROL_MODULE_ID, "Control task starting" );

    /* Wait for sensors to be ready before processing */
    xEventGroupWaitBits( g_evSystemFlags,
                         EVT_SENSOR_READY,
                         pdFALSE, pdTRUE,
                         pdMS_TO_TICKS( 10000U ) );

    for( ;; )
    {
        /* Block on sensor queue with timeout matching control period */
        if( xQueueReceive( g_qSensorData,
                           &sensorMsg,
                           pdMS_TO_TICKS( CONTROL_LOOP_MS * 2U ) ) == pdTRUE )
        {
            /* Run control algorithm */
            control_output = PID_Update( &s_pid, sensorMsg.temperature, dt_s );

            /* TODO: Write output to actuator driver */
            /* drv_pwm_set_duty( PWM_CH0, (uint16_t)control_output ); */

            ( void ) control_output;
        }
        else
        {
            /* Sensor data timeout - safe state */
            TaskManager_Log( LOG_WARN, CONTROL_MODULE_ID,
                             "Sensor data timeout - holding last output" );
        }

        /* Process any pending commands (non-blocking) */
        while( xQueueReceive( g_qCommandQueue, &cmdMsg, 0 ) == pdTRUE )
        {
            switch( cmdMsg.cmd_id )
            {
                case 0x01U: /* Set setpoint */
                    memcpy( &s_pid.setpoint, cmdMsg.payload, sizeof( float ) );
                    TaskManager_Log( LOG_INFO, CONTROL_MODULE_ID,
                                     "Setpoint updated: %.2f", s_pid.setpoint );
                    break;

                case 0x02U: /* Reset PID integrator */
                    s_pid.integral   = 0.0f;
                    s_pid.prev_error = 0.0f;
                    break;

                default:
                    TaskManager_Log( LOG_WARN, CONTROL_MODULE_ID,
                                     "Unknown command: 0x%02X", cmdMsg.cmd_id );
                    break;
            }
        }
    }
}
