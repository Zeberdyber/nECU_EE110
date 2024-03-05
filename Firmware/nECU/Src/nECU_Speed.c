/**
 ******************************************************************************
 * @file    nECU_Speed.c
 * @brief   This file provides code for wheel speed sensors.
 ******************************************************************************
 */

#include "nECU_Speed.h"

// internal variables
static Speed_Sensor Speed_Sens_1, Speed_Sens_2, Speed_Sens_3, Speed_Sens_4;
static CalibrateRoutine calibrateRoutine;
// initialized flags
static bool SS1_Initialized = false, SS2_Initialized = false, SS3_Initialized = false, SS4_Initialized = false;
// external import
extern uint16_t *ADC_V1, *ADC_V2, *ADC_V3, *ADC_V4; // ADC variables

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

/* Averaging */
void Speed_AverageCalc(Speed_Sensor *Sensor) // routine to calculate averaged speed
{
    Sensor->Average.Buffer[Sensor->Average.BufferIndex] = Sensor->SpeedData;
    Sensor->Average.BufferIndex++;
    if (Sensor->Average.BufferIndex > SPEED_AVERAGE_BUFFER_SIZE)
    {
        Sensor->Average.BufferIndex = 0;
    }
    else if (Sensor->Average.BufferIndex == SPEED_AVERAGE_BUFFER_SIZE) // if buffer is full then calculate average
    {
        uint32_t AverageTemporary = 0;
        for (uint8_t i = 0; i < SPEED_AVERAGE_BUFFER_SIZE; i++)
        {
            AverageTemporary += Sensor->Average.Buffer[i];
        }
        Sensor->SpeedDataSlow = AverageTemporary / SPEED_AVERAGE_BUFFER_SIZE;
    }
}
void Speed_AverageInit(Speed_Sensor *Sensor) // Initialize averaging structure
{
    for (uint8_t i = 0; i < SPEED_AVERAGE_BUFFER_SIZE; i++)
    {
        Sensor->Average.Buffer[i] = 0;
    }
    Sensor->Average.BufferIndex = 0;
}

/* General functions */
void Speed_Start(void) // function to start Speed function set
{
    // Setup sensor objects input
    Speed_Sens_1.InputData = ADC_V1;
    Speed_Sens_2.InputData = ADC_V2;
    Speed_Sens_3.InputData = ADC_V3;
    Speed_Sens_4.InputData = ADC_V4;

    // // Read calibraion from flash
    nECU_readSpeedCalibration(&Speed_Sens_1.SensorCorrection, &Speed_Sens_2.SensorCorrection, &Speed_Sens_3.SensorCorrection, &Speed_Sens_4.SensorCorrection);

    // // Setup base parameters before first can recived frame
    Speed_Sens_1.WheelSetup = nECU_CAN_getWheelSetupPointer();
    Speed_Sens_2.WheelSetup = nECU_CAN_getWheelSetupPointer();
    Speed_Sens_3.WheelSetup = nECU_CAN_getWheelSetupPointer();
    Speed_Sens_4.WheelSetup = nECU_CAN_getWheelSetupPointer();

    // other init
    Speed_Sens_1.SpeedData = 0;
    Speed_Sens_2.SpeedData = 0;
    Speed_Sens_3.SpeedData = 0;
    Speed_Sens_4.SpeedData = 0;

    SS1_Initialized = true;
    SS2_Initialized = true;
    SS3_Initialized = true;
    SS4_Initialized = true;
}
void Speed_Update(void) // perform update of all sensors
{
    if (SS1_Initialized == true)
    {
        Speed_SensorUpdate(&Speed_Sens_1);
    }
    if (SS2_Initialized == true)
    {
        Speed_SensorUpdate(&Speed_Sens_2);
    }
    if (SS3_Initialized == true)
    {
        Speed_SensorUpdate(&Speed_Sens_3);
    }
    if (SS4_Initialized == true)
    {
        Speed_SensorUpdate(&Speed_Sens_4);
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
    if (calibrateRoutine.initialized == true) // do only when calibrating
    {
        Speed_AverageCalc(Sensor);
    }
}
void Speed_CorrectToCalib(Speed_Sensor *Sensor) // correct data to calibration multiplier
{
    Sensor->SpeedData = Sensor->SpeedData * Sensor->SensorCorrection;
}
void Speed_ADCToSpeed(Speed_Sensor *Sensor) // function to convert RAW ADC data to real speed in km/h
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
void Speed_CalibrateSingle(Speed_Sensor *Sensor) // function to generate calibration multiplier
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
void Speed_CalibrateAll(void) // function to calibrate speed sensors (periodic function)
{
    if (calibrateRoutine.initialized == false)
    {
        Speed_AverageInit(&Speed_Sens_1);
        Speed_AverageInit(&Speed_Sens_2);
        Speed_AverageInit(&Speed_Sens_3);
        Speed_AverageInit(&Speed_Sens_4);
        calibrateRoutine.initialized = true;
    }
    else
    {
        if (Speed_Sens_1.Average.BufferIndex == SPEED_AVERAGE_BUFFER_SIZE)
        {
            calibrateRoutine.averageReady[0] = true;
        }
        if (Speed_Sens_2.Average.BufferIndex == SPEED_AVERAGE_BUFFER_SIZE)
        {
            calibrateRoutine.averageReady[1] = true;
        }
        if (Speed_Sens_3.Average.BufferIndex == SPEED_AVERAGE_BUFFER_SIZE)
        {
            calibrateRoutine.averageReady[2] = true;
        }
        if (Speed_Sens_4.Average.BufferIndex == SPEED_AVERAGE_BUFFER_SIZE)
        {
            calibrateRoutine.averageReady[3] = true;
        }
        if (calibrateRoutine.averageReady[0] && calibrateRoutine.averageReady[1] && calibrateRoutine.averageReady[2] && calibrateRoutine.averageReady[3])
        {
            Speed_CalibrateSingle(&Speed_Sens_1);
            Speed_CalibrateSingle(&Speed_Sens_2);
            Speed_CalibrateSingle(&Speed_Sens_3);
            Speed_CalibrateSingle(&Speed_Sens_4);
            nECU_saveSpeedCalibration(&Speed_Sens_1.SensorCorrection, &Speed_Sens_2.SensorCorrection, &Speed_Sens_3.SensorCorrection, &Speed_Sens_4.SensorCorrection);
            Speed_CalibrateInit(); // clear calibration data
        }
    }
}
void Speed_CalibrateInit(void) // initialize calibration structure
{
    // calibration structure initialization to default values
    calibrateRoutine.active = false;
    calibrateRoutine.initialized = false;
    calibrateRoutine.averageReady[0] = false;
    calibrateRoutine.averageReady[1] = false;
    calibrateRoutine.averageReady[2] = false;
    calibrateRoutine.averageReady[3] = false;
}
void Speed_CalibrateStart(void) // start calibration process
{
    Speed_CalibrateInit();
    calibrateRoutine.active = true;
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
