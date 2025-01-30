/**
 ******************************************************************************
 * @file    nECU_Speed.c
 * @brief   This file provides code for wheel speed sensors.
 ******************************************************************************
 */

#include "nECU_Speed.h"

// internal variables
static Speed_Sensor Speed_Sens_1 = {0}, Speed_Sens_2 = {0}, Speed_Sens_3 = {0}, Speed_Sens_4 = {0};
static CalibrateRoutine calibrateRoutine = {0};

extern nECU_ProgramBlockData D_SS1, D_SS2, D_SS3, D_SS4; // diagnostic and flow control data

/* Interface functions */
uint16_t *Speed_GetSpeed(Speed_Sensor_ID ID) // get current speed
{
    switch (ID)
    {
    case 1:
        return &Speed_Sens_1.SpeedData;
        break;
    case 2:
        return &Speed_Sens_2.SpeedData;
        break;
    case 3:
        return &Speed_Sens_3.SpeedData;
        break;
    case 4:
        return &Speed_Sens_4.SpeedData;
        break;

    default:
        return 0;
        break;
    }
}
uint16_t *Speed_GetSpeedSlow(Speed_Sensor_ID ID) // get slower (average) speed
{
    switch (ID)
    {
    case 1:
        return &Speed_Sens_1.SpeedDataSlow;
        break;
    case 2:
        return &Speed_Sens_2.SpeedDataSlow;
        break;
    case 3:
        return &Speed_Sens_3.SpeedDataSlow;
        break;
    case 4:
        return &Speed_Sens_4.SpeedDataSlow;
        break;

    default:
        return 0;
        break;
    }
}
void Speed_SetWheelSetup(uint8_t WheelSetup) // set wheel setup for all sensors
{
    if (WheelSetup == 1 || WheelSetup == 2)
    {
        Speed_Sens_1.WheelCirc = WheelSetup;
        Speed_Sens_2.WheelCirc = WheelSetup;
        Speed_Sens_3.WheelCirc = WheelSetup;
        Speed_Sens_4.WheelCirc = WheelSetup;
    }
}

/* General functions */
bool Speed_Start(void) // function to start Speed function set
{
    bool status = false;

    // Read calibraion from flash
    status |= nECU_readSpeedCalibration(&Speed_Sens_1.SensorCorrection, &Speed_Sens_2.SensorCorrection, &Speed_Sens_3.SensorCorrection, &Speed_Sens_4.SensorCorrection);

    if (D_SS1.Status == D_BLOCK_STOP)
    {
        status |= Speed_Init_Single(&Speed_Sens_1, SPEED_SENSOR_FRONT_LEFT);
        D_SS1.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_SS1.Status & D_BLOCK_INITIALIZED)
    {
        status |= ADC2_START();
        D_SS1.Status |= D_BLOCK_WORKING;
    }

    if (D_SS2.Status == D_BLOCK_STOP)
    {
        status |= Speed_Init_Single(&Speed_Sens_2, SPEED_SENSOR_FRONT_RIGHT);
        D_SS2.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_SS2.Status & D_BLOCK_INITIALIZED)
    {
        status |= ADC2_START();
        D_SS2.Status |= D_BLOCK_WORKING;
    }

    if (D_SS3.Status == D_BLOCK_STOP)
    {
        status |= Speed_Init_Single(&Speed_Sens_3, SPEED_SENSOR_REAR_LEFT);
        D_SS3.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_SS3.Status & D_BLOCK_INITIALIZED)
    {
        status |= ADC2_START();
        D_SS3.Status |= D_BLOCK_WORKING;
    }

    if (D_SS4.Status == D_BLOCK_STOP)
    {
        status |= Speed_Init_Single(&Speed_Sens_4, SPEED_SENSOR_REAR_RIGHT);
        D_SS4.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_SS4.Status & D_BLOCK_INITIALIZED)
    {
        status |= ADC2_START();
        D_SS4.Status |= D_BLOCK_WORKING;
    }

    return status;
}
static bool Speed_Init_Single(Speed_Sensor *Sensor, Speed_Sensor_ID id) // initializes structure for single sensor
{
    Sensor->id = id;
    Sensor->InputData = getPointer_SpeedSens_ADC(id);
    Sensor->WheelSetup = nECU_CAN_getWheelSetupPointer();
    Sensor->SpeedData = 0;
    Sensor->Average.BufferIndex = 0;
    return false;
}
static void Speed_Update(void) // perform update of all sensors
{
    if (D_SS1.Status & D_BLOCK_WORKING)
    {
        Speed_SensorUpdate(&Speed_Sens_1);
        nECU_Debug_ProgramBlockData_Update(&D_SS1);
    }
    if (D_SS2.Status & D_BLOCK_WORKING)
    {
        Speed_SensorUpdate(&Speed_Sens_2);
        nECU_Debug_ProgramBlockData_Update(&D_SS2);
    }
    if (D_SS3.Status & D_BLOCK_WORKING)
    {
        Speed_SensorUpdate(&Speed_Sens_3);
        nECU_Debug_ProgramBlockData_Update(&D_SS3);
    }
    if (D_SS4.Status & D_BLOCK_WORKING)
    {
        Speed_SensorUpdate(&Speed_Sens_4);
        nECU_Debug_ProgramBlockData_Update(&D_SS4);
    }
}
void Speed_SensorUpdate(Speed_Sensor *Sensor) // update one sensors data
{
    // Update wheel circumference
    switch (*Sensor->WheelSetup)
    {
    case 1:
        Sensor->WheelCirc = Wheel_Circ_Set_1;
        break;
    case 2:
        Sensor->WheelCirc = Wheel_Circ_Set_2;
        break;

    default:
        Sensor->WheelCirc = 1000; // 1 m
        break;
    }

    Speed_ADCToSpeed(Sensor);
    Speed_CorrectToCalib(Sensor);
    nECU_averageSmooth(&(Sensor->Average.Buffer[0]), &(Sensor->SpeedData), &(Sensor->SpeedDataSlow), sizeof(Sensor->Average.Buffer) / sizeof(uint16_t));

    if (Sensor->Average.BufferIndex < SPEED_AVERAGE_BUFFER_SIZE)
    {
        Sensor->Average.BufferIndex++;
    }
    else
    {
        calibrateRoutine.averageReady[Sensor->id] = true;
    }
}
static void Speed_CorrectToCalib(Speed_Sensor *Sensor) // correct data to calibration multiplier
{
    Sensor->SpeedData = Sensor->SpeedData * Sensor->SensorCorrection;
}
static void Speed_ADCToSpeed(Speed_Sensor *Sensor) // function to convert RAW ADC data to real speed in km/h
{
    float Speed = 0;
    Speed = (ADCToVolts(*Sensor->InputData) * 3.6 * Speed_HzTomVolts * Sensor->WheelCirc) / Speed_ToothCount; // 3.6 to convert mm/s to km/h

    for (uint8_t i = 0; i < Speed_DecimalPoint; i++) // move decimal point before float to uint16_t cast
    {
        Speed = Speed * 10;
    }
    Speed -= SPEED_OFFSET;
    if (Speed > (float)UINT16_MAX || Speed < 0) // cut off when over limit
    {
        Speed = 0.0;
    }
    uint16_t speed_now = Speed;

    // smooth the signal
    nECU_ADC_expSmooth(&(speed_now), &(Sensor->SpeedData), 1, SPEED_EXP_ALPHA); // 1 for only one variable to smooth
}

/* Calibration functions */
static void Speed_CalibrateSingle(Speed_Sensor *Sensor) // function to generate calibration multiplier
{
    // Ride with defined (by ) constant speed  ! according to GPS !

    uint16_t CalibrationSpeed = SPEED_CALIB_VELOCITY;
    uint16_t CurrentSpeed = Sensor->SpeedDataSlow;

    for (uint8_t i = 0; i < Speed_DecimalPoint; i++) // move by decimal point
    {
        CalibrationSpeed *= 10;
    }
    float result = (float)CalibrationSpeed / CurrentSpeed;
    Sensor->SensorCorrection = result;
}
static void Speed_CalibrateAll(void) // function to calibrate speed sensors (periodic function)
{
    if (!(D_SS1.Status & D_SS2.Status & D_SS3.Status & D_SS4.Status & D_BLOCK_WORKING)) // Check if all are working
    {
        return;
    }

    if (calibrateRoutine.averageReady[0] && calibrateRoutine.averageReady[1] && calibrateRoutine.averageReady[2] && calibrateRoutine.averageReady[3])
    {
        Speed_CalibrateSingle(&Speed_Sens_1);
        Speed_CalibrateSingle(&Speed_Sens_2);
        Speed_CalibrateSingle(&Speed_Sens_3);
        Speed_CalibrateSingle(&Speed_Sens_4);
        nECU_saveSpeedCalibration(&Speed_Sens_1.SensorCorrection, &Speed_Sens_2.SensorCorrection, &Speed_Sens_3.SensorCorrection, &Speed_Sens_4.SensorCorrection);
        Speed_CalibrateInit(); // clear calibration data
        calibrateRoutine.active = false;
    }
}
static void Speed_CalibrateInit(void) // initialize calibration structure
{
    // calibration structure initialization to default values
    calibrateRoutine.active = true;
    Speed_Sens_1.Average.BufferIndex = 0;
    Speed_Sens_2.Average.BufferIndex = 0;
    Speed_Sens_3.Average.BufferIndex = 0;
    Speed_Sens_4.Average.BufferIndex = 0;
    calibrateRoutine.averageReady[0] = false;
    calibrateRoutine.averageReady[1] = false;
    calibrateRoutine.averageReady[2] = false;
    calibrateRoutine.averageReady[3] = false;
}
void Speed_CalibrateStart(void) // start calibration process
{
    if (!(D_SS1.Status & D_SS2.Status & D_SS3.Status & D_SS4.Status & D_BLOCK_WORKING)) // Check if all are working
    {
        return;
    }

    Speed_CalibrateInit();
}

/* Periodic functions */
void Speed_TimingEvent(void) // function to be called periodicaly with desired data update rate
{
    if (calibrateRoutine.active)
    {
        Speed_CalibrateAll();
    }
    Speed_Update();
}
