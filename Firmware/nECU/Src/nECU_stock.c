/**
 ******************************************************************************
 * @file    nECU_stock.c
 * @brief   This file provides code for handling of stock Toyota functions.
 ******************************************************************************
 */

#include "nECU_stock.h"

extern uint16_t *ADC_MAP, *ADC_BackPressure, *ADC_OX;

AnalogSensor MAP;
AnalogSensor BackPressure;
Oxygen OX;
VSS_Handle VSS;
uint8_t spd = 0;

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
    MAP.calibrationData.ADC_MeasuredMin = MAP_ADC_CALIB_MIN;
    MAP.calibrationData.ADC_MeasuredMax = MAP_ADC_CALIB_MAX;
    MAP.calibrationData.OUT_MeasuredMax = MAP_kPA_CALIB_MAX;
    MAP.calibrationData.OUT_MeasuredMin = MAP_kPA_CALIB_MIN;
    nECU_calculateLinearCalibration(&MAP.calibrationData);
    MAP.decimalPoint = MAP_DECIMAL_POINT;
    MAP.ADC_input = ADC_MAP;
    MAP.initialized = true;
}
void nECU_MAP_Update(void) // update MAP structure
{
    if (MAP.initialized == false)
    {
        nECU_MAP_Init();
    }
    else
    {
        MAP.outputFloat = nECU_getLinearSensor(MAP.ADC_input, &MAP.calibrationData);
        MAP.output16bit = nECU_FloatToUint16_t(MAP.outputFloat, MAP_DECIMAL_POINT, 10);
    }
}
/* BackPressure */
uint8_t *nECU_BackPressure_GetPointer(void) // returns pointer to resulting data
{
    return &BackPressure.output8bit;
}
void nECU_BackPressure_Init(void) // initialize BackPressure structure
{
    BackPressure.calibrationData.ADC_MeasuredMin = BACKPRESSURE_ADC_CALIB_MIN;
    BackPressure.calibrationData.ADC_MeasuredMax = BACKPRESSURE_ADC_CALIB_MAX;
    BackPressure.calibrationData.OUT_MeasuredMax = BACKPRESSURE_kPA_CALIB_MAX;
    BackPressure.calibrationData.OUT_MeasuredMin = BACKPRESSURE_kPA_CALIB_MIN;
    nECU_calculateLinearCalibration(&BackPressure.calibrationData);
    BackPressure.decimalPoint = BACKPRESSURE_DECIMAL_POINT;
    BackPressure.ADC_input = ADC_BackPressure;
    BackPressure.initialized = true;
}
void nECU_BackPressure_Update(void) // update BackPressure structure
{
    if (BackPressure.initialized == false)
    {
        nECU_BackPressure_Init();
    }
    else
    {
        BackPressure.outputFloat = nECU_getLinearSensor(BackPressure.ADC_input, &BackPressure.calibrationData);
        BackPressure.output8bit = nECU_FloatToUint8_t(BackPressure.outputFloat, BACKPRESSURE_DECIMAL_POINT, 8);
    }
}
/* Oxygen */
uint8_t *nECU_OX_GetPointer(void) // returns pointer to resulting data
{
    return &OX.sensor.output8bit;
}
void nECU_OX_Init(void) // initialize narrowband lambda structure
{
    /* Sensor */
    OX.sensor.calibrationData.ADC_MeasuredMax = VoltsToADC(OXYGEN_VOLTAGE_CALIB_MAX);
    OX.sensor.calibrationData.ADC_MeasuredMin = VoltsToADC(OXYGEN_VOLTAGE_CALIB_MIN);
    OX.sensor.calibrationData.OUT_MeasuredMax = OXYGEN_VOLTAGE_MAX;
    OX.sensor.calibrationData.OUT_MeasuredMin = OXYGEN_VOLTAGE_MIN;
    nECU_calculateLinearCalibration(&OX.sensor.calibrationData);
    OX.sensor.decimalPoint = OXYGEN_DECIMAL_POINT;
    OX.sensor.ADC_input = ADC_OX;
    OX.sensor.initialized = true;
    /* Heater */
    OX.Heater.Timer = &OX_HEATER_TIMER;
    OX.Heater.Channel = 1;
    HAL_TIM_Base_Start_IT(OX.Heater.Timer);
    HAL_TIM_PWM_Start_IT(OX.Heater.Timer, TIM_CHANNEL_1);
    OX.Heater.Infill = 0;
    OX.Coolant = nECU_CAN_getCoolantPointer();
    OX.Heater.Timer->Instance->CCR1 = 0;
    OX.Infill_max = OXYGEN_HEATER_MAX;
    OX.Infill_min = OXYGEN_HEATER_MIN;
    OX.Coolant_max = OXYGEN_COOLANT_MAX;
    OX.Coolant_min = OXYGEN_COOLANT_MIN;
}
void nECU_OX_Update(void) // update narrowband lambda structure
{
    /* Sensor update */
    if (OX.sensor.initialized == false)
    {
        nECU_BackPressure_Init();
    }
    else
    {
        OX.sensor.outputFloat = nECU_getLinearSensor(OX.sensor.ADC_input, &OX.sensor.calibrationData);
        OX.sensor.output8bit = nECU_FloatToUint8_t(OX.sensor.outputFloat, OX.sensor.decimalPoint, 8);
    }

    /* Output update */
    float coolantFloat = *OX.Coolant;
    /* simple algorithm that linearly scale heater voltage with engine coolant temperature */
    OX.Heater.Infill = nECU_Table_Interpolate(&OX.Coolant_min, &OX.Infill_max, &OX.Coolant_max, &OX.Infill_min, &coolantFloat);
    OX.Heater.Timer->Instance->CCR1 = (OX.Heater.Infill * OX.Heater.Timer->Init.Period) / 100;
}
void nECU_OX_DeInit(void) // deinitialize narrowband lambda structure
{
    HAL_TIM_Base_Stop_IT(OX.Heater.Timer);
    HAL_TIM_PWM_Stop_IT(OX.Heater.Timer, TIM_CHANNEL_1);
}
/* VSS - Vehicle Speed Sensor */
uint8_t *nECU_VSS_GetPointer() // returns pointer to resulting data
{
    return &VSS.Speed;
}
void nECU_VSS_Init(void) // initialize VSS structure
{
    VSS.VSS_prevCCR = 0;
    VSS.tim.htim = &FREQ_INPUT_TIMER;
    VSS.VSS_Channel = TIM_CHANNEL_2;
    VSS.tim.refClock = TIM_CLOCK / (VSS.tim.htim->Init.Prescaler + 1);
    VSS.watchdogCount = 0;
    HAL_TIM_Base_Start_IT(VSS.tim.htim);
    HAL_TIM_IC_Start_IT(VSS.tim.htim, VSS.VSS_Channel);
}
void nECU_VSS_Update(void) // update VSS structure
{
    uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(VSS.tim.htim, VSS.VSS_Channel);

    /* Calculate difference */
    uint16_t Difference = 0; // in miliseconds
    if (VSS.VSS_prevCCR > CurrentCCR)
    {
        Difference = ((VSS.tim.htim->Init.Period + 1 - VSS.VSS_prevCCR) + CurrentCCR);
    }
    else
    {
        Difference = (CurrentCCR - VSS.VSS_prevCCR);
    }
    VSS.VSS_prevCCR = CurrentCCR;
    VSS.frequency = VSS.tim.refClock / Difference;
    float speed = (VSS.frequency) * (3600.0f / VSS_PULSES_PER_KM);
    if (speed > 0xFF)
    {
        speed = 0xFF;
    }
    else if (speed < 0)
    {
        speed = 0;
    }

    VSS.Speed = (uint8_t)speed;
    VSS.watchdogCount = 0;
}
void nECU_VSS_DetectZero(TIM_HandleTypeDef *htim) // detect if zero km/h
{
    float time = (TIM_CLOCK / (htim->Init.Prescaler + 1)) / (htim->Init.Period + 1);
    if (VSS.Speed != 0)
    {
        VSS.watchdogCount++;
        if ((VSS.watchdogCount / time) > (VSS.tim.htim->Init.Period / VSS.tim.refClock))
        {
            VSS.Speed = 0;
        }
    }
}
void nECU_VSS_DeInit(void) // deinitialize VSS structure
{
    HAL_TIM_Base_Stop_IT(VSS.tim.htim);
    HAL_TIM_IC_Stop_IT(VSS.tim.htim, VSS.VSS_Channel);
}
/* General */
void nECU_Stock_Start(void) // function to initialize all stock stuff
{
    nECU_MAP_Init();
    nECU_BackPressure_Init();
    nECU_OX_Init();
    nECU_VSS_Init();
}
void nECU_Stock_Stop(void) // function to deinitialize all stock stuff
{
    nECU_OX_DeInit();
    nECU_VSS_DeInit();
}
void nECU_Stock_Update(void) // function to update structures
{
    nECU_BackPressure_Update();
    nECU_MAP_Update();
    nECU_OX_Update();
}