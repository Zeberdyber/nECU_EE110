/**
 ******************************************************************************
 * @file    nECU_Speed.c
 * @brief   This file provides code for wheel speed sensors.
 ******************************************************************************
 */

#include "nECU_Speed.h"

Speed_Sensor Speed_Sens_1, Speed_Sens_2, Speed_Sens_3, Speed_Sens_4;

extern uint16_t *ADC_V1, *ADC_V2, *ADC_V3, *ADC_V4; // ADC variables

CalibrateRoutine calibrateRoutine;

/* Interface functions */
uint16_t *Speed_GetSpeed(uint8_t sensorNumber) // get current speed
{
    switch (sensorNumber)
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
uint16_t *Speed_GetSpeedSlow(uint8_t sensorNumber) // get slower (average) speed
{
    switch (sensorNumber)
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

    // averaging initialization
    Speed_AverageInit(&Speed_Sens_1);
    Speed_AverageInit(&Speed_Sens_2);
    Speed_AverageInit(&Speed_Sens_3);
    Speed_AverageInit(&Speed_Sens_4);

    // other init
    Speed_Sens_1.SpeedData = 0;
    Speed_Sens_2.SpeedData = 0;
    Speed_Sens_3.SpeedData = 0;
    Speed_Sens_4.SpeedData = 0;
}
void Speed_Update(void) // perform update of all sensors
{
    Speed_SensorUpdate(&Speed_Sens_1);
    Speed_SensorUpdate(&Speed_Sens_2);
    Speed_SensorUpdate(&Speed_Sens_3);
    Speed_SensorUpdate(&Speed_Sens_4);
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
    Speed_AverageCalc(Sensor);
}
void Speed_CorrectToCalib(Speed_Sensor *Sensor) // correct data to calibration multiplier
{
    Sensor->SpeedData = Sensor->SpeedData * Sensor->SensorCorrection;
}
void Speed_ADCToSpeed(Speed_Sensor *Sensor) // function to convert RAW ADC data to real speed in km/h
{
    Sensor->SpeedDataPrev = Sensor->SpeedData;
    float Speed = 0;
    Speed = (ADCToVolts(*Sensor->InputData) * 3.6 * Speed_HzTomVolts * Sensor->WheelCirc) / Speed_ToothCount;

    for (uint8_t i = 0; i < Speed_DecimalPoint; i++) // move decimal point before float to uint16_t cast
    {
        Speed = Speed * 10;
    }
    Speed -= SPEED_OFFSET;
    if (Speed > 65535 || Speed < 0) // cut off when over limit
    {
        Speed = 0.0;
    }
    Sensor->SpeedData = Sensor->SpeedDataPrev + SPEED_EXP_ALPHA * ((uint16_t)Speed - Sensor->SpeedDataPrev);
}

/* Calibration functions */
void Speed_CalibrateSingle(Speed_Sensor *Sensor) // function to generate calibration multiplier
{
    // Ride 50km/h  ! according to GPS !

    uint16_t CalibrationSpeed = 50;
    uint16_t CurrentSpeed = Sensor->SpeedDataSlow;

    for (uint8_t i = 0; i < Speed_DecimalPoint; i++)
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
            Speed_CalibrateInit();
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

/* Speed testing functions */
uint8_t Test_Speed_SensorUpdate(void) // function to test Speed functions
{
    // Success values
    uint16_t Corr_Circ = Wheel_Circ_Set_1; // according to definitions
    uint16_t Corr_Speed = 15000;           // 150km/h

    uint16_t Input = VoltsToADC(((Corr_Speed / 100) * Speed_ToothCount) / (3.6 * Wheel_Circ_Set_1 * Speed_HzTomVolts)); // ADC value to plug into test, calculated from known Voltage

    // Create test object
    Speed_Sensor Test_Obj;
    Test_Obj.InputData = &Input;
    Test_Obj.WheelSetup = (uint8_t *)1;
    Test_Obj.SensorCorrection = 1.0;

    // Perfrom tests
    Speed_SensorUpdate(&Test_Obj);

    // Check results
    if (Test_Obj.WheelCirc != Corr_Circ)
    {
        return 1;
    }
    if (Test_Obj.SpeedData - Corr_Speed > 50) // Allow for 0.5km/h error due to rounding
    {
        return 1;
    }

    return 0;
}
