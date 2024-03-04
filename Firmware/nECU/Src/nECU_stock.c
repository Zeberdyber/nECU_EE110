/**
 ******************************************************************************
 * @file    nECU_stock.c
 * @brief   This file provides code for handling of stock Toyota functions.
 ******************************************************************************
 */

#include "nECU_stock.h"

// internal variables
static AnalogSensor_Handle MAP;
static AnalogSensor_Handle BackPressure;
Oxygen_Handle OX;
VSS_Handle VSS;
IGF_Handle IGF;
static stock_GPIO stk_in;

// initialized flags
static bool MAP_Initialized = false, BackPressure_Initialized = false, OX_Initialized = false, VSS_Initialized = false, IGF_Initialized = false;
// external import
extern uint16_t *ADC_MAP, *ADC_BackPressure, *ADC_OX;

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
    MAP_Initialized = true;
}
void nECU_MAP_Update(void) // update MAP structure
{
    if (MAP_Initialized == false)
    {
        nECU_MAP_Init();
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
    BackPressure.calibrationData.ADC_MeasuredMin = BACKPRESSURE_ADC_CALIB_MIN;
    BackPressure.calibrationData.ADC_MeasuredMax = BACKPRESSURE_ADC_CALIB_MAX;
    BackPressure.calibrationData.OUT_MeasuredMax = BACKPRESSURE_kPA_CALIB_MAX;
    BackPressure.calibrationData.OUT_MeasuredMin = BACKPRESSURE_kPA_CALIB_MIN;
    nECU_calculateLinearCalibration(&BackPressure.calibrationData);
    BackPressure.decimalPoint = BACKPRESSURE_DECIMAL_POINT;
    BackPressure.ADC_input = ADC_BackPressure;
    BackPressure_Initialized = true;
}
void nECU_BackPressure_Update(void) // update BackPressure structure
{
    if (BackPressure_Initialized == false)
    {
        nECU_BackPressure_Init();
        return;
    }
    BackPressure.outputFloat = nECU_getLinearSensor(BackPressure.ADC_input, &BackPressure.calibrationData);
    BackPressure.output8bit = nECU_FloatToUint8_t(BackPressure.outputFloat, BACKPRESSURE_DECIMAL_POINT, 8);
}
/* Oxygen Sensor */
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
    /* Heater */
    // timer configuration
    OX.Heater.htim = &OX_HEATER_TIMER;
    nECU_tim_Init_struct(&(OX.Heater));
    OX.Heater.Channel_Count = 1;
    OX.Heater.Channel_List[0] = TIM_CHANNEL_1;
    nECU_tim_PWM_start(&(OX.Heater));
    // variables configuration
    OX.Heater_Infill = 0;
    OX.Coolant = nECU_CAN_getCoolantPointer();
    OX.Infill_max = OXYGEN_HEATER_MAX;
    OX.Infill_min = OXYGEN_HEATER_MIN;
    OX.Coolant_max = OXYGEN_COOLANT_MAX;
    OX.Coolant_min = OXYGEN_COOLANT_MIN;

    OX_Initialized = true;
}
void nECU_OX_Update(void) // update narrowband lambda structure
{
    /* Sensor update */
    if (OX_Initialized == false)
    {
        nECU_OX_Init();
        return;
    }
    OX.sensor.outputFloat = nECU_getLinearSensor(OX.sensor.ADC_input, &OX.sensor.calibrationData);
    OX.sensor.output8bit = nECU_FloatToUint8_t(OX.sensor.outputFloat, OX.sensor.decimalPoint, 8);

    /* Output update */
    /* simple algorithm that linearly scale heater voltage with engine coolant temperature */
    float coolant = (float)*OX.Coolant;
    OX.Heater_Infill = nECU_Table_Interpolate(&OX.Coolant_min, &OX.Infill_max, &OX.Coolant_max, &OX.Infill_min, &coolant);
    nECU_OX_PWM_Set(&(OX.Heater_Infill));
}
void nECU_OX_DeInit(void) // deinitialize narrowband lambda structure
{
    if (OX_Initialized == true)
    {
        nECU_tim_PWM_stop(&(OX.Heater));
    }
}
void nECU_OX_PWM_Set(float *infill) // function to set PWM according to set infill
{
    OX.Heater.htim->Instance->CCR1 = (*infill * OX.Heater.htim->Init.Period) / 100;
}
/* VSS - Vehicle Speed Sensor */
uint8_t *nECU_VSS_GetPointer() // returns pointer to resulting data
{
    return &VSS.Speed;
}
void nECU_VSS_Init(void) // initialize VSS structure
{
    VSS.ic.previous_CCR = 0;
    VSS.tim.htim = &FREQ_INPUT_TIMER;
    nECU_tim_Init_struct(&VSS.tim);
    VSS.tim.Channel_Count = 1;
    VSS.tim.Channel_List[0] = TIM_CHANNEL_2;
    nECU_tim_IC_start(&VSS.tim);
    VSS_Initialized = true;
}
void nECU_VSS_Update(void) // update VSS structure
{
    if (VSS_Initialized == false) // check if initialized
    {
        nECU_VSS_Init();
        return;
    }

    float speed = (VSS.ic.frequency) * (3600.0f / VSS_PULSES_PER_KM); // 3600 for m/s to km/h
    if (speed > (float)UINT8_MAX)
    {
        speed = UINT8_MAX;
    }
    else if (speed < 0)
    {
        speed = 0;
    }

    VSS.Speed = (uint8_t)speed;
}
void nECU_VSS_DetectZero(TIM_HandleTypeDef *htim) // detect if zero km/h -- !!! to be fixed
{
    // float time = (TIM_CLOCK / (htim->Init.Prescaler + 1)) / (htim->Init.Period + 1);
    // if (VSS.Speed != 0)
    // {
    //     VSS.watchdogCount++;
    //     if ((VSS.watchdogCount / time) > (VSS.tim.htim->Init.Period / VSS.tim.refClock))
    //     {
    //         VSS.Speed = 0;
    //     }
    // }
}
void nECU_VSS_DeInit(void) // deinitialize VSS structure
{
    if (VSS_Initialized == true)
    {
        HAL_TIM_Base_Stop_IT(VSS.tim.htim);
        HAL_TIM_IC_Stop_IT(VSS.tim.htim, VSS.tim.Channel_List[0]);
    }
}
/* IGF - Ignition feedback */
void nECU_IGF_Init(void) // initialize and start
{
    IGF.ic.previous_CCR = 0;
    IGF.tim.htim = &FREQ_INPUT_TIMER;
    nECU_tim_Init_struct(&IGF.tim);
    IGF.tim.Channel_Count = 1;
    IGF.tim.Channel_List[0] = TIM_CHANNEL_1;
    nECU_tim_IC_start(&IGF.tim);
    IGF_Initialized = true;
}
void nECU_IGF_Calc(void) // calculate RPM based on IGF signal
{
    if (IGF_Initialized == false) // check if initialized
    {
        nECU_IGF_Init();
        return;
    }

    uint16_t RPM = IGF.ic.frequency * 120;
    if (RPM > IGF_MAX_RPM)
    {
        return;
    }

    float rpm_rate = RPM - IGF.RPM;
    if (rpm_rate < 0)
    {
        rpm_rate = -rpm_rate;
    }
    rpm_rate *= IGF.ic.frequency;
    if (rpm_rate > IGF_MAX_RPM_RATE)
    {
        nECU_Fault_Missfire();
    }
    IGF.RPM = RPM; // save current RPM
}
void nECU_IGF_DeInit(void) // stop
{
    if (IGF_Initialized == true)
    {
        HAL_TIM_Base_Stop_IT(IGF.tim.htim);
        HAL_TIM_IC_Stop_IT(IGF.tim.htim, IGF.tim.Channel_List[0]);
    }
}
/* GPIO inputs */
void nECU_stock_GPIO_Init(void) // initialize structure variables
{
    stk_in.Cranking.GPIO_Pin = Cranking_Pin;
    stk_in.Cranking.GPIOx = Cranking_GPIO_Port;

    stk_in.Fan_ON.GPIO_Pin = Fan_ON_Pin;
    stk_in.Fan_ON.GPIOx = Fan_ON_GPIO_Port;

    stk_in.Lights_ON.GPIO_Pin = Lights_ON_Pin;
    stk_in.Lights_ON.GPIOx = Lights_ON_GPIO_Port;
}
void nECU_stock_GPIO_update(void) // update structure variables
{
    stk_in.Cranking.State = HAL_GPIO_ReadPin(stk_in.Cranking.GPIOx, stk_in.Cranking.GPIO_Pin);
    stk_in.Fan_ON.State = HAL_GPIO_ReadPin(stk_in.Fan_ON.GPIOx, stk_in.Fan_ON.GPIO_Pin);
    stk_in.Lights_ON.State = HAL_GPIO_ReadPin(stk_in.Lights_ON.GPIOx, stk_in.Lights_ON.GPIO_Pin);
    stk_in.Cranking_b = (bool)stk_in.Cranking.State;
    stk_in.Fan_ON_b = (bool)stk_in.Fan_ON.State;
    stk_in.Lights_ON_b = (bool)stk_in.Lights_ON.State;
}
bool *nECU_stock_GPIO_getPointer(stock_inputs_ID id) // return pointers of structure variables
{
    switch (id)
    {
    case INPUT_CRANKING_ID:
        return &stk_in.Cranking_b;
        break;
    case INPUT_FAN_ON_ID:
        return &stk_in.Fan_ON_b;
        break;
    case INPUT_LIGHTS_ON_ID:
        return &stk_in.Lights_ON_b;
        break;

    default:
        break;
    }
    return NULL;
}
/* Immobilizer */
bool none = true;
bool *nECU_Immo_getPointer(void) // returns pointer to immobilizer valid
{
    return &none;
}
/* General */
void nECU_Stock_Start(void) // function to initialize all stock stuff
{
    nECU_MAP_Init();
    nECU_BackPressure_Init();
    nECU_OX_Init();
    nECU_VSS_Init();
    nECU_IGF_Init();
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
    nECU_VSS_Update();
    nECU_IGF_Calc();
}