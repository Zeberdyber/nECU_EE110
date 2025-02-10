/**
 ******************************************************************************
 * @file    nECU_stock.c
 * @brief   This file provides code for handling of stock Toyota functions.
 ******************************************************************************
 */

#include "nECU_stock.h"

// internal variables
static Oxygen_Handle OX = {0};
static stock_GPIO Stock_GPIO = {0};

/* Oxygen Sensor */
uint8_t *nECU_OX_GetPointer(void) // returns pointer to resulting data
{
    return &OX.sensor.output8bit;
}
bool nECU_OX_Init(void) // initialize narrowband lambda structure
{
    bool status = false;

    if (D_OX.Status == D_BLOCK_STOP)
    {
        /* Sensor */
        status |= nECU_A_Input_Init(&(OX.sensor), VoltsToADC(OXYGEN_VOLTAGE_CALIB_MAX), VoltsToADC(OXYGEN_VOLTAGE_CALIB_MIN), OXYGEN_VOLTAGE_MAX, OXYGEN_VOLTAGE_MIN, nECU_ADC_getPointer_OX());
        /* Heater */
        // timer configuration
        OX.Heater.htim = &OX_HEATER_TIMER;
        nECU_TIM_Init(&(OX.Heater));
        OX.Heater.Channel_Count = 1;
        OX.Heater.Channel_List[0] = TIM_CHANNEL_1;
        // variables configuration
        OX.Heater_Infill = 0;
        OX.Coolant = nECU_CAN_getPointer_Coolant();
        OX.Infill_max = OXYGEN_HEATER_MAX;
        OX.Infill_min = OXYGEN_HEATER_MIN;
        OX.Coolant_max = OXYGEN_COOLANT_MAX;
        OX.Coolant_min = OXYGEN_COOLANT_MIN;

        D_OX.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_OX.Status & D_BLOCK_INITIALIZED)
    {
        status |= (nECU_TIM_PWM_Start(&(OX.Heater)) != TIM_OK);
        status |= nECU_ADC1_START();
        D_OX.Status |= D_BLOCK_WORKING;
    }

    return status;
}
void nECU_OX_Update(void) // update narrowband lambda structure
{
    /* Sensor update */
    if (!(D_OX.Status & D_BLOCK_WORKING))
    {
        D_OX.Status |= D_BLOCK_CODE_ERROR;
        return;
    }
    nECU_InputAnalog_ADC1_Routine(ADC1_OX_ID);

    /* Output update */
    /* simple algorithm that linearly scale heater voltage with engine coolant temperature */
    float coolant = (float)*OX.Coolant;
    OX.Heater_Infill = nECU_Table_Interpolate(&OX.Coolant_min, &OX.Infill_max, &OX.Coolant_max, &OX.Infill_min, &coolant);
    nECU_OX_PWM_Set(&(OX.Heater_Infill));

    nECU_Debug_ProgramBlockData_Update(&D_OX);
}
void nECU_OX_DeInit(void) // deinitialize narrowband lambda structure
{
    if (D_OX.Status & D_BLOCK_INITIALIZED)
    {
        nECU_TIM_PWM_Stop(&(OX.Heater));
        D_OX.Status -= D_BLOCK_INITIALIZED_WORKING;
    }
}
static void nECU_OX_PWM_Set(float *infill) // function to set PWM according to set infill
{
    OX.Heater.htim->Instance->CCR1 = (*infill * (OX.Heater.htim->Init.Period + 1)) / 100;
}
/* GPIO inputs */
bool nECU_stock_GPIO_Init(void) // initialize structure variables
{
    bool status = false;

    if (D_GPIO.Status == D_BLOCK_STOP)
    {
        Stock_GPIO.Cranking.GPIO_Pin = Cranking_Pin;
        Stock_GPIO.Cranking.GPIOx = Cranking_GPIO_Port;

        Stock_GPIO.Fan_ON.GPIO_Pin = Fan_ON_Pin;
        Stock_GPIO.Fan_ON.GPIOx = Fan_ON_GPIO_Port;

        Stock_GPIO.Lights_ON.GPIO_Pin = Lights_ON_Pin;
        Stock_GPIO.Lights_ON.GPIOx = Lights_ON_GPIO_Port;

        D_GPIO.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_GPIO.Status & D_BLOCK_INITIALIZED)
    {
        D_GPIO.Status |= D_BLOCK_WORKING;
    }

    return status;
}
void nECU_stock_GPIO_update(void) // update structure variables
{
    if (!(D_GPIO.Status & D_BLOCK_WORKING))
    {
        D_GPIO.Status |= D_BLOCK_CODE_ERROR;
        return;
    }

    Stock_GPIO.Cranking.State = HAL_GPIO_ReadPin(Stock_GPIO.Cranking.GPIOx, Stock_GPIO.Cranking.GPIO_Pin);
    Stock_GPIO.Fan_ON.State = HAL_GPIO_ReadPin(Stock_GPIO.Fan_ON.GPIOx, Stock_GPIO.Fan_ON.GPIO_Pin);
    Stock_GPIO.Lights_ON.State = HAL_GPIO_ReadPin(Stock_GPIO.Lights_ON.GPIOx, Stock_GPIO.Lights_ON.GPIO_Pin);
    Stock_GPIO.Cranking_b = (bool)Stock_GPIO.Cranking.State;
    Stock_GPIO.Fan_ON_b = (bool)Stock_GPIO.Fan_ON.State;
    Stock_GPIO.Lights_ON_b = (bool)Stock_GPIO.Lights_ON.State;

    nECU_Debug_ProgramBlockData_Update(D_GPIO);
}
bool *nECU_stock_GPIO_getPointer(stock_inputs_ID id) // return pointers of structure variables
{
    switch (id)
    {
    case INPUT_CRANKING_ID:
        return &Stock_GPIO.Cranking_b;
        break;
    case INPUT_FAN_ON_ID:
        return &Stock_GPIO.Fan_ON_b;
        break;
    case INPUT_LIGHTS_ON_ID:
        return &Stock_GPIO.Lights_ON_b;
        break;

    default:
        D_GPIO.Status |= D_BLOCK_CODE_ERROR;
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

/* GPIO */
bool nECU_GPIO_Init(GPIO_struct *inst, uint16_t Pin, GPIO_TypeDef *Port) // Initialize structure - clear
{
    if (inst == NULL) // Check if pointer exist
        return true;  // Break
    if (Port == NULL) // Check if pointer exist
        return true;  // Break

    inst->GPIO_Pin = Pin;
    inst->GPIOx = Port;
    HAL_GPIO_Init(inst->GPIOx, inst->GPIO_Pin);
    return false;
}
bool nECU_GPIO_Read(GPIO_struct *inst) // Read value and save to structure
{
    if (inst == NULL) // Check if pointer exist
        return true;  // Break

    if (inst->GPIOx == NULL) // Check if was initialized
        return true;         // Break

    inst->State = HAL_GPIO_ReadPin(inst->GPIOx, inst->GPIO_Pin);
    return false;
}
bool nECU_GPIO_Write(GPIO_struct *inst, bool state) // Write value
{
    if (inst == NULL) // Check if pointer exist
        return true;  // Break

    if (inst->GPIOx == NULL) // Check if was initialized
        return true;         // Break

    inst->State = (GPIO_PinState)state;
    HAL_GPIO_WritePin(inst->GPIOx, inst->GPIO_Pin, inst->State);
    return false;
}