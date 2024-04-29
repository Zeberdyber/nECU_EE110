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
        nECU_A_Input_Init(&(BackPressure), BACKPRESSURE_ADC_CALIB_MAX, BACKPRESSURE_ADC_CALIB_MIN, BACKPRESSURE_kPA_CALIB_MAX, BACKPRESSURE_kPA_CALIB_MIN, getPointer_Backpressure_ADC());
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
    nECU_A_Input_Update(&(BackPressure));
    BackPressure.output8bit = nECU_FloatToUint8_t(BackPressure.outputFloat, BACKPRESSURE_DECIMAL_POINT, 8);
}

/* Addtional Analog inputs sketch */
void nECU_A_Input_Init(AnalogSensor_Handle *sensor, uint16_t inMax, uint16_t inMin, float outMax, float outMin, uint16_t *ADC_Data) // function to setup the structure
{
    // setup basic parameters
    sensor->calibrationData.ADC_MeasuredMax = inMax;
    sensor->calibrationData.ADC_MeasuredMin = inMin;
    sensor->calibrationData.OUT_MeasuredMax = outMax;
    sensor->calibrationData.OUT_MeasuredMin = outMin;
    sensor->ADC_input = ADC_Data;
    nECU_calculateLinearCalibration(&(sensor->calibrationData));

    // setup filter and delay
    sensor->filter.smoothingAlpha = 1.0;
    sensor->filter.smoothingBufferLen = 0;
    uint32_t start_delay = 0;
    nECU_Delay_Set(&(sensor->filter.delay), &start_delay);

    // set default values to the outputs
    sensor->outputFloat = 0.0;
    sensor->output8bit = 0;
    sensor->output16bit = 0;
}
void nECU_A_Input_SetSmoothing(AnalogSensor_Handle *sensor, float alpha, uint16_t *smoothing_buffer, uint8_t buffer_length, uint16_t update_frequency) // setups filtering and smoothing
{
    if (update_frequency > 0) // set delay if requested
    {
        uint32_t delay = 1000 / update_frequency;
        nECU_Delay_Set(&(sensor->filter.delay), &delay);
        nECU_Delay_Start(&(sensor->filter.delay));
    }
    // copy smoothing data
    sensor->filter.smoothingAlpha = alpha;
    sensor->filter.smoothingBuffer = smoothing_buffer;
    sensor->filter.smoothingBufferLen = buffer_length;
}
void nECU_A_Input_Update(AnalogSensor_Handle *sensor) // update current value
{
    uint16_t ADC_Data = sensor->output16bit; // set data as default to previous value (for filtering)

    if (sensor->filter.delay.active == true) // check if delay was setup
    {
        nECU_Delay_Update(&(sensor->filter.delay));
        if (sensor->filter.delay.done == true) // check if time have passed
        {
            nECU_Delay_Start(&(sensor->filter.delay)); // restart delay
            // rest of the function will be done
        }
        else
        {
            return; // drop if not done
        }
    }

    // do the filtering if needed
    if (sensor->filter.smoothingAlpha < 1.0) // check if alpha was configured
    {
        nECU_expSmooth((sensor->ADC_input), &ADC_Data, sensor->filter.smoothingAlpha);
    }
    if (sensor->filter.smoothingBufferLen > 0) // check if buffer was configured
    {
        nECU_averageSmooth((sensor->filter.smoothingBuffer), &ADC_Data, &ADC_Data, (sensor->filter.smoothingBufferLen));
    }

    sensor->outputFloat = nECU_getLinearSensor(&ADC_Data, &(sensor->calibrationData));
    sensor->output16bit = (uint16_t)sensor->outputFloat;
    sensor->output8bit = (uint8_t)sensor->outputFloat;
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
