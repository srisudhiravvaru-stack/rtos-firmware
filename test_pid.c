/**
 * @file test_pid.c
 * @brief Unit Tests for PID Controller
 *
 * Tests the PID control algorithm in task_control.c using the
 * Unity test framework. Run on host (no hardware required).
 */

#include "unity.h"
#include <math.h>
#include <string.h>

/* ---- Expose internal PID struct for white-box testing ---- */
typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
    float setpoint;
    float output_min;
    float output_max;
} PID_t;

/* Forward declaration of function under test */
static float PID_Update( PID_t *pid, float measurement, float dt_s );

/* Copy of PID_Update for host testing (normally in task_control.c) */
static float PID_Update( PID_t *pid, float measurement, float dt_s )
{
    float error    = pid->setpoint - measurement;
    pid->integral += error * dt_s;
    if( pid->integral > pid->output_max ) pid->integral = pid->output_max;
    if( pid->integral < pid->output_min ) pid->integral = pid->output_min;
    float derivative = ( error - pid->prev_error ) / dt_s;
    pid->prev_error  = error;
    float output = ( pid->kp * error ) + ( pid->ki * pid->integral ) + ( pid->kd * derivative );
    if( output > pid->output_max ) output = pid->output_max;
    if( output < pid->output_min ) output = pid->output_min;
    return output;
}

/* ---------------------------------------------------------------------------
 * Test Fixtures
 *---------------------------------------------------------------------------*/
static PID_t s_pid;

void setUp( void )
{
    memset( &s_pid, 0, sizeof( s_pid ) );
    s_pid.kp         = 1.0f;
    s_pid.ki         = 0.1f;
    s_pid.kd         = 0.05f;
    s_pid.setpoint   = 25.0f;
    s_pid.output_min = -100.0f;
    s_pid.output_max = 100.0f;
}

void tearDown( void ) { /* nothing */ }

/* ---------------------------------------------------------------------------
 * Tests
 *---------------------------------------------------------------------------*/

void test_PID_NoError_OutputZero( void )
{
    /* When measurement equals setpoint, output should be ~0 */
    float out = PID_Update( &s_pid, 25.0f, 0.01f );
    TEST_ASSERT_FLOAT_WITHIN( 0.001f, 0.0f, out );
}

void test_PID_PositiveError_PositiveOutput( void )
{
    /* Measurement below setpoint -> positive (corrective) output */
    float out = PID_Update( &s_pid, 20.0f, 0.01f );
    TEST_ASSERT_GREATER_THAN_FLOAT( 0.0f, out );
}

void test_PID_NegativeError_NegativeOutput( void )
{
    float out = PID_Update( &s_pid, 30.0f, 0.01f );
    TEST_ASSERT_LESS_THAN_FLOAT( 0.0f, out );
}

void test_PID_OutputClamped_MaxLimit( void )
{
    /* Large positive error should clamp output to output_max */
    float out = PID_Update( &s_pid, -10000.0f, 0.01f );
    TEST_ASSERT_FLOAT_WITHIN( 0.001f, 100.0f, out );
}

void test_PID_OutputClamped_MinLimit( void )
{
    float out = PID_Update( &s_pid, 10000.0f, 0.01f );
    TEST_ASSERT_FLOAT_WITHIN( 0.001f, -100.0f, out );
}

void test_PID_IntegralWindupPrevented( void )
{
    /* Run many iterations with saturated error - integral must be clamped */
    for( int i = 0; i < 1000; i++ )
    {
        PID_Update( &s_pid, -10000.0f, 0.01f );
    }
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT( 100.0f, s_pid.integral );
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT( -100.0f, s_pid.integral );
}

void test_PID_ConvergesOnSetpoint( void )
{
    /* Simulate a simple first-order system to check PID drives error down */
    float plant_state = 0.0f;
    float dt          = 0.01f;

    for( int i = 0; i < 500; i++ )
    {
        float u  = PID_Update( &s_pid, plant_state, dt );
        plant_state += u * dt * 0.5f;  /* Simple integrating plant model */
    }

    /* After 5 seconds, should be near setpoint */
    TEST_ASSERT_FLOAT_WITHIN( 2.0f, s_pid.setpoint, plant_state );
}

/* ---------------------------------------------------------------------------
 * Test Runner
 *---------------------------------------------------------------------------*/
int main( void )
{
    UNITY_BEGIN();

    RUN_TEST( test_PID_NoError_OutputZero );
    RUN_TEST( test_PID_PositiveError_PositiveOutput );
    RUN_TEST( test_PID_NegativeError_NegativeOutput );
    RUN_TEST( test_PID_OutputClamped_MaxLimit );
    RUN_TEST( test_PID_OutputClamped_MinLimit );
    RUN_TEST( test_PID_IntegralWindupPrevented );
    RUN_TEST( test_PID_ConvergesOnSetpoint );

    return UNITY_END();
}
