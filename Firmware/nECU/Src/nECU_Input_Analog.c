/**
 ******************************************************************************
 * @file    nECU_Input_Analog.c
 * @brief   This file provides code for analog inputs.
 ******************************************************************************
 */

#include "nECU_Input_Analog.h"

// internal variables
static AnalogSensor_Handle MAP = {0};
static AnalogSensor_Handle BackPressure = {0};
static nECU_InternalTemp MCU_temperature = {0};
static AnalogSensor_Handle AI1 = {0}, AI2 = {0}, AI3 = {0}; // additional analog inputs

extern nECU_ProgramBlockData D_MAP, D_BackPressure, D_MCU_temperature, D_AdditionalAI, D_Input_Analog; // diagnostic and flow control data

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

/* Internal Temperatre (MCU) */
bool nECU_InternalTemp_Init(void) // initialize structure
{
    bool status = false;

    if (D_MCU_temperature.Status == D_BLOCK_STOP)
    {
        MCU_temperature.ADC_data = getPointer_InternalTemp_ADC();
        MCU_temperature.temperature = 0;
        D_MCU_temperature.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_MCU_temperature.Status & D_BLOCK_INITIALIZED)
    {
        nECU_InternalTemp_Delay_Start();
        nECU_InternalTemp_StartupDelay_Start();
        status |= ADC1_START();
        printf("Internal MCU temperature -> STARTED!\n");
        D_MCU_temperature.Status |= D_BLOCK_WORKING;
    }

    return status;
}
void nECU_InternalTemp_Callback(void) // run when conversion ended
{
    if (!(D_MCU_temperature.Status & D_BLOCK_WORKING) || *nECU_InternalTemp_StartupDelay_DoneFlag() == false)
    {
        return;
    }
    if (*nECU_InternalTemp_Delay_DoneFlag() == true)
    {
        nECU_InternalTemp_Update(); // calculate value
        nECU_InternalTemp_Delay_Start();
    }
}
void nECU_InternalTemp_Update(void) // perform update of output variables
{
    if (!(D_MCU_temperature.Status & D_BLOCK_WORKING))
    {
        return;
    }

    // convert to temperature
    float Temperature = ADCToVolts(*MCU_temperature.ADC_data);
    Temperature -= (float)INTERNAL_TEMP_V25;
    Temperature /= (float)(INTERNAL_TEMP_SLOPE / 1000); // 1000: mV -> V
    Temperature += (float)25;
    MCU_temperature.temperature = Temperature * INTERNAL_TEMP_MULTIPLIER;

    nECU_Debug_ProgramBlockData_Update(&D_MCU_temperature);
}
int16_t *nECU_InternalTemp_getTemperature(void) // return current temperature pointer (multiplied 100x)
{
    /* returned value is multipled 100 times, which means that it carries two digits after dot */
    return &MCU_temperature.temperature;
}

/* MAP */
uint16_t *nECU_MAP_GetPointer(void) // returns pointer to resulting data
{
    return &MAP.output16bit;
}
bool nECU_MAP_Init(void) // initialize MAP structure
{
    bool status = false;

    if (D_MAP.Status == D_BLOCK_STOP)
    {
        status |= nECU_A_Input_Init(&MAP, MAP_ADC_CALIB_MAX, MAP_ADC_CALIB_MIN, MAP_kPA_CALIB_MAX, MAP_kPA_CALIB_MIN, getPointer_MAP_ADC());
        D_MAP.Status = D_BLOCK_INITIALIZED;
    }
    if (D_MAP.Status == D_BLOCK_INITIALIZED)
    {
        status |= ADC1_START();
        printf("Stock MAP -> STARTED!\n");
        D_MAP.Status = D_BLOCK_WORKING;
    }
    return status;
}
void nECU_MAP_Update(void) // update MAP structure
{
    if (D_MAP.Status != D_BLOCK_WORKING)
    {
        return;
    }
    nECU_A_Input_Update(&MAP);
    MAP.output16bit = nECU_FloatToUint16_t(MAP.outputFloat, MAP_DECIMAL_POINT, 10); // manually update due to 10bit MAP resolution in CAN frame

    nECU_Debug_ProgramBlockData_Update(&D_MAP);
}

/* BackPressure */
uint8_t *nECU_BackPressure_GetPointer(void) // returns pointer to resulting data
{
    return &BackPressure.output8bit;
}
bool nECU_BackPressure_Init(void) // initialize BackPressure structure
{
    bool status = false;

    if (D_BackPressure.Status == D_BLOCK_STOP)
    {
        status |= nECU_A_Input_Init(&(BackPressure), BACKPRESSURE_ADC_CALIB_MAX, BACKPRESSURE_ADC_CALIB_MIN, BACKPRESSURE_kPA_CALIB_MAX, BACKPRESSURE_kPA_CALIB_MIN, getPointer_Backpressure_ADC());
        D_BackPressure.Status = D_BLOCK_INITIALIZED;
    }
    if (D_BackPressure.Status == D_BLOCK_INITIALIZED)
    {
        status |= ADC1_START();
        printf("Backpressure sensing -> STARTED!\n");
        D_BackPressure.Status = D_BLOCK_WORKING;
    }

    return status;
}
void nECU_BackPressure_Update(void) // update BackPressure structure
{
    if (D_BackPressure.Status != D_BLOCK_WORKING)
    {
        return;
    }
    nECU_A_Input_Update(&(BackPressure));
    BackPressure.output8bit = nECU_FloatToUint8_t(BackPressure.outputFloat, BACKPRESSURE_DECIMAL_POINT, 8);

    nECU_Debug_ProgramBlockData_Update(&D_BackPressure);
}

/* Addtional Analog inputs */
void nECU_A_Input_Init_All(void)
{
    if (D_AdditionalAI.Status == D_BLOCK_STOP)
    {
        // Example values to initialize inputs [temporary values]
        uint16_t inMax = ADC_MAX_VALUE_12BIT;
        uint16_t inMin = 0;
        float outMax = 100; // 100%
        float outMin = 0;   // 0%

        // Initialize all
        nECU_A_Input_Init(&AI1, inMax, inMin, outMax, outMin, getPointer_AnalogInput(ANALOG_IN_1));
        nECU_A_Input_Init(&AI2, inMax, inMin, outMax, outMin, getPointer_AnalogInput(ANALOG_IN_2));
        nECU_A_Input_Init(&AI3, inMax, inMin, outMax, outMin, getPointer_AnalogInput(ANALOG_IN_3));
        D_AdditionalAI.Status = D_BLOCK_INITIALIZED;
    }
    if (D_AdditionalAI.Status == D_BLOCK_INITIALIZED)
    {
        ADC1_START();
        printf("Additional AI -> STARTED!\n");
        D_AdditionalAI.Status = D_BLOCK_WORKING;
    }
}
void nECU_A_Input_Update_All(void)
{
    if (D_AdditionalAI.Status != D_BLOCK_WORKING)
    {
        return;
    }
    nECU_A_Input_Update(&AI1);
    nECU_A_Input_Update(&AI2);
    nECU_A_Input_Update(&AI3);

    nECU_Debug_ProgramBlockData_Update(&D_AdditionalAI);
}
bool nECU_A_Input_Init(AnalogSensor_Handle *sensor, uint16_t inMax, uint16_t inMin, float outMax, float outMin, uint16_t *ADC_Data) // function to setup the structure
{
    bool status = false;

    // setup basic parameters
    sensor->calibrationData.ADC_MeasuredMax = inMax;
    sensor->calibrationData.ADC_MeasuredMin = inMin;
    sensor->calibrationData.OUT_MeasuredMax = outMax;
    sensor->calibrationData.OUT_MeasuredMin = outMin;
    sensor->ADC_input = ADC_Data;
    nECU_calculateLinearCalibration(&(sensor->calibrationData));

    // setup filter and delay [disable by default]
    sensor->filter.smoothingAlpha = 1.0;
    sensor->filter.smoothingBufferLen = 0;
    uint32_t start_delay = 0;
    nECU_Delay_Set(&(sensor->filter.delay), &start_delay);

    // set default values to the outputs
    sensor->outputFloat = 0.0;
    sensor->output8bit = 0;
    sensor->output16bit = 0;

    return status;
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
    uint16_t ADC_Data = *(sensor->ADC_input); // variable to be used by smoothing algorithms

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
        nECU_expSmooth((sensor->ADC_input), &(sensor->filter.previous_ADC_Data), &ADC_Data, sensor->filter.smoothingAlpha);
    }
    if (sensor->filter.smoothingBufferLen > 0) // check if buffer was configured
    {
        nECU_averageSmooth((sensor->filter.smoothingBuffer), &ADC_Data, &ADC_Data, (sensor->filter.smoothingBufferLen));
    }

    sensor->filter.previous_ADC_Data = ADC_Data; // save smoothing output
    sensor->outputFloat = nECU_getLinearSensor(&ADC_Data, &(sensor->calibrationData));
    sensor->output16bit = (uint16_t)sensor->outputFloat;
    sensor->output8bit = (uint8_t)sensor->outputFloat;
}

/* General */
void nECU_Analog_Start(void) // start of all analog functions
{
    nECU_InternalTemp_Init();
    nECU_MAP_Init();
    nECU_BackPressure_Init();
    nECU_A_Input_Init_All();
}
void nECU_Analog_Stop(void) // stop of all analog functions
{
    UNUSED(false);
}
void nECU_Analog_Update(void) // update to all analog functions
{
    nECU_MAP_Update();
    nECU_BackPressure_Update();
    nECU_InternalTemp_Callback();
    nECU_A_Input_Update_All();

    nECU_Debug_ProgramBlockData_Update(&D_Input_Analog);
}
