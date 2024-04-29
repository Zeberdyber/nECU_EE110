/**
 ******************************************************************************
 * @file    nECU_stock.c
 * @brief   This file provides code for handling of stock Toyota functions.
 ******************************************************************************
 */

#include "nECU_stock.h"

// internal variables
Oxygen_Handle OX;
static stock_GPIO stk_in;

// initialized flags
static bool OX_Initialized = false, OX_Working = false,
            GPIO_Initialized = false, GPIO_Working = false;

/* Oxygen Sensor */
uint8_t *nECU_OX_GetPointer(void) // returns pointer to resulting data
{
    return &OX.sensor.output8bit;
}
void nECU_OX_Init(void) // initialize narrowband lambda structure
{
    if (OX_Initialized == false)
    {
        /* Sensor */
        OX.sensor.calibrationData.ADC_MeasuredMax = VoltsToADC(OXYGEN_VOLTAGE_CALIB_MAX);
        OX.sensor.calibrationData.ADC_MeasuredMin = VoltsToADC(OXYGEN_VOLTAGE_CALIB_MIN);
        OX.sensor.calibrationData.OUT_MeasuredMax = OXYGEN_VOLTAGE_MAX;
        OX.sensor.calibrationData.OUT_MeasuredMin = OXYGEN_VOLTAGE_MIN;
        nECU_calculateLinearCalibration(&OX.sensor.calibrationData);
        OX.sensor.ADC_input = getPointer_OX_ADC();
        /* Heater */
        // timer configuration
        OX.Heater.htim = &OX_HEATER_TIMER;
        nECU_tim_Init_struct(&(OX.Heater));
        OX.Heater.Channel_Count = 1;
        OX.Heater.Channel_List[0] = TIM_CHANNEL_1;
        // variables configuration
        OX.Heater_Infill = 0;
        OX.Coolant = nECU_CAN_getCoolantPointer();
        OX.Infill_max = OXYGEN_HEATER_MAX;
        OX.Infill_min = OXYGEN_HEATER_MIN;
        OX.Coolant_max = OXYGEN_COOLANT_MAX;
        OX.Coolant_min = OXYGEN_COOLANT_MIN;

        OX_Initialized = true;
    }
    if (OX_Working == false && OX_Initialized == true)
    {
        nECU_tim_PWM_start(&(OX.Heater));
        ADC1_START();
        OX_Working = true;
    }
}
void nECU_OX_Update(void) // update narrowband lambda structure
{
    /* Sensor update */
    if (OX_Initialized == false || OX_Working == false)
    {
        return;
    }

    OX.sensor.outputFloat = nECU_getLinearSensor(OX.sensor.ADC_input, &OX.sensor.calibrationData);
    OX.sensor.output8bit = nECU_FloatToUint8_t(OX.sensor.outputFloat, OXYGEN_DECIMAL_POINT, 8);

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
    OX.Heater.htim->Instance->CCR1 = (*infill * (OX.Heater.htim->Init.Period + 1)) / 100;
}
/* GPIO inputs */
void nECU_stock_GPIO_Init(void) // initialize structure variables
{
    if (GPIO_Initialized == false)
    {
        stk_in.Cranking.GPIO_Pin = Cranking_Pin;
        stk_in.Cranking.GPIOx = Cranking_GPIO_Port;

        stk_in.Fan_ON.GPIO_Pin = Fan_ON_Pin;
        stk_in.Fan_ON.GPIOx = Fan_ON_GPIO_Port;

        stk_in.Lights_ON.GPIO_Pin = Lights_ON_Pin;
        stk_in.Lights_ON.GPIOx = Lights_ON_GPIO_Port;
    }
    if (GPIO_Working == false && GPIO_Initialized == true)
    {
        GPIO_Working = true;
    }
}
void nECU_stock_GPIO_update(void) // update structure variables
{
    if (GPIO_Initialized == false || GPIO_Working == false)
    {
        return;
    }
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
    nECU_OX_Init();
}
void nECU_Stock_Stop(void) // function to deinitialize all stock stuff
{
    nECU_OX_DeInit();
}
void nECU_Stock_Update(void) // function to update structures
{
    nECU_OX_Update();
    nECU_stock_GPIO_update();
}