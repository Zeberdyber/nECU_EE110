/**
 ******************************************************************************
 * @file    nECU_Input_Analog.c
 * @brief   This file provides code for analog inputs.
 ******************************************************************************
 */

#include "nECU_Input_Analog.h"

// internal variables
static AnalogSensor_Handle MAP;
static AnalogSensor_Handle BackPressure;

// initialized flags
static bool MAP_Initialized = false, MAP_Working = false,
            BackPressure_Initialized = false, BackPressure_Working = false;

/* Analog sensors */
void nECU_calculateLinearCalibration(AnalogSensorCalibration *inst) // function to calculate factor (a) and offset (b) for linear formula: y=ax+b
{
    inst->factor = (float)(inst->OUT_MeasuredMax - inst->OUT_MeasuredMin) / (inst->ADC_MeasuredMax - inst->ADC_MeasuredMin);
    inst->offset = (float)inst->OUT_MeasuredMax - (inst->factor * inst->ADC_MeasuredMax);
}
float nECU_getLinearSensor(uint16_t *ADC_Value, AnalogSensorCalibration *inst) // function to get result of linear sensor
{
    return *ADC_Value * inst->factor + inst->offset;
}
uint16_t nECU_FloatToUint16_t(float input, uint8_t decimalPoint, uint8_t bitCount) // convert float value to uint16_t value with correct decimal point and bit lenght
{
    uint16_t output;
    for (uint8_t i = 0; i < decimalPoint; i++) // adjust decimal point
    {
        input *= 10;
    }
    output = (uint16_t)input;

    uint16_t mask = 0;
    for (uint8_t i = 0; i <= bitCount; i++) // form mask for bit count
    {
        mask = (mask << 1) + 1;
    }

    return output & mask;
}
uint8_t nECU_FloatToUint8_t(float input, uint8_t decimalPoint, uint8_t bitCount) // convert float value to uint8_t value with correct decimal point and bit lenght
{
    uint8_t output;
    for (uint8_t i = 0; i < decimalPoint; i++) // adjust decimal point
    {
        input *= 10;
    }
    output = (uint16_t)input;

    uint8_t mask = 0;
    for (uint8_t i = 0; i <= bitCount; i++) // form mask for bit count
    {
        mask = (mask << 1) + 1;
    }

    return output & mask;
}
/* MAP */
uint16_t *nECU_MAP_GetPointer(void) // returns pointer to resulting data
{
    return &MAP.output16bit;
}
void nECU_MAP_Init(void) // initialize MAP structure
{
    if (MAP_Initialized == false)
    {
        MAP.calibrationData.ADC_MeasuredMin = MAP_ADC_CALIB_MIN;
        MAP.calibrationData.ADC_MeasuredMax = MAP_ADC_CALIB_MAX;
        MAP.calibrationData.OUT_MeasuredMax = MAP_kPA_CALIB_MAX;
        MAP.calibrationData.OUT_MeasuredMin = MAP_kPA_CALIB_MIN;
        nECU_calculateLinearCalibration(&MAP.calibrationData);
        MAP.decimalPoint = MAP_DECIMAL_POINT;
        MAP.ADC_input = getPointer_MAP_ADC();
        MAP_Initialized = true;
    }
    if (MAP_Working == false && MAP_Initialized == true)
    {
        ADC1_START();
        MAP_Working = true;
    }
}
void nECU_MAP_Update(void) // update MAP structure
{
    if (MAP_Working == false)
    {
        return;
    }
    MAP.outputFloat = nECU_getLinearSensor(MAP.ADC_input, &MAP.calibrationData);
    MAP.output16bit = nECU_FloatToUint16_t(MAP.outputFloat, MAP_DECIMAL_POINT, 10);
}
/* BackPressure */
uint8_t *nECU_BackPressure_GetPointer(void) // returns pointer to resulting data
{
    return &BackPressure.output8bit;
}
void nECU_BackPressure_Init(void) // initialize BackPressure structure
{
    if (BackPressure_Initialized == false)
    {
        BackPressure.calibrationData.ADC_MeasuredMin = BACKPRESSURE_ADC_CALIB_MIN;
        BackPressure.calibrationData.ADC_MeasuredMax = BACKPRESSURE_ADC_CALIB_MAX;
        BackPressure.calibrationData.OUT_MeasuredMax = BACKPRESSURE_kPA_CALIB_MAX;
        BackPressure.calibrationData.OUT_MeasuredMin = BACKPRESSURE_kPA_CALIB_MIN;
        nECU_calculateLinearCalibration(&BackPressure.calibrationData);
        BackPressure.decimalPoint = BACKPRESSURE_DECIMAL_POINT;
        BackPressure.ADC_input = getPointer_Backpressure_ADC();
        BackPressure_Initialized = true;
    }
    if (BackPressure_Working == false && BackPressure_Initialized == true)
    {
        ADC1_START();
        BackPressure_Working = true;
    }
}
void nECU_BackPressure_Update(void) // update BackPressure structure
{
    if (BackPressure_Working == false)
    {
        return;
    }
    BackPressure.outputFloat = nECU_getLinearSensor(BackPressure.ADC_input, &BackPressure.calibrationData);
    BackPressure.output8bit = nECU_FloatToUint8_t(BackPressure.outputFloat, BACKPRESSURE_DECIMAL_POINT, 8);
}

/* General */
void nECU_Analog_Start(void) // start of all analog functions
{
    nECU_MAP_Init();
    nECU_BackPressure_Init();
}

void nECU_Analog_Stop(void) // stop of all analog functions
{
    UNUSED(false);
}

void nECU_Analog_Update(void) // update to all analog functions
{
    nECU_MAP_Update();
    nECU_BackPressure_Update();
}
