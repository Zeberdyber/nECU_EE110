/**
 ******************************************************************************
 * @file    nECU_stock.c
 * @brief   This file provides code for handling of stock Toyota functions.
 ******************************************************************************
 */

#include "nECU_stock.h"

// internal variables
static Oxygen_Handle OX = {0};

static GPIO_struct Input_List[DigiInput_ID_MAX] = {0};

static uint16_t Input_Pin_List[DigiInput_ID_MAX] = {
    [DigiInput_CRANKING_ID] = Cranking_Pin,
    [DigiInput_FAN_ON_ID] = Fan_ON_Pin,
    [DigiInput_LIGHTS_ON_ID] = Lights_ON_Pin,
    [DigiInput_VSS_ID] = GPIO_PIN_13,
    [DigiInput_IGF_ID] = GPIO_PIN_12,
}; // List of Pins
static GPIO_TypeDef *Input_Port_List[DigiInput_ID_MAX] = {
    [DigiInput_CRANKING_ID] = Cranking_GPIO_Port,
    [DigiInput_FAN_ON_ID] = Fan_ON_GPIO_Port,
    [DigiInput_LIGHTS_ON_ID] = Lights_ON_GPIO_Port,
    [DigiInput_VSS_ID] = GPIOD,
    [DigiInput_IGF_ID] = GPIOD,
}; // List of Ports

/* Oxygen Sensor */
bool nECU_OX_Start(void) // initialize narrowband lambda structure
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_OX))
    {
        // variables configuration
        OX.Heater_Infill = 0;
        OX.Infill_max = OXYGEN_HEATER_MAX;
        OX.Infill_min = OXYGEN_HEATER_MIN;
        OX.Coolant_max = OXYGEN_COOLANT_MAX;
        OX.Coolant_min = OXYGEN_COOLANT_MIN;
        status |= nECU_CAN_Start();

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_OX);
    }
    if (!nECU_FlowControl_Working_Check(D_OX) && status == false)
    {
        status |= nECU_InputAnalog_ADC1_Start(ADC1_OX_ID);
        status |= nECU_TIM_PWM_Start(TIM_PWM_OX_ID, 0);
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_OX);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_OX);

    return status;
}
void nECU_OX_Routine(void) // update narrowband lambda structure
{
    if (!nECU_FlowControl_Working_Check(D_OX)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_OX);
        return; // Break
    }

    /* Input sensor */
    nECU_InputAnalog_ADC1_Routine(ADC1_OX_ID);

    /* Output update */
    /* simple algorithm that linearly scale heater voltage with engine coolant temperature */
    float coolant = nECU_CAN_RX_getValue(CAN_RX_Coolant_ID);
    // OX.Heater_Infill = nECU_Table_Interpolate(&OX.Coolant_min, &OX.Infill_max, &OX.Coolant_max, &OX.Infill_min, &coolant);
    // OX.Heater.htim->Instance->CCR1 = (OX.Heater_Infill * (OX.Heater.htim->Init.Period + 1)) / 100;

    nECU_Debug_ProgramBlockData_Update(D_OX);
}
bool nECU_OX_Stop(void) // deinitialize narrowband lambda structure
{
    bool status = false;

    if (nECU_FlowControl_Working_Check(D_OX) && status == false)
    {
        status |= nECU_TIM_PWM_Stop(TIM_PWM_OX_ID, TIM_CHANNEL_1);
        status |= nECU_InputAnalog_ADC1_Stop(ADC1_OX_ID);
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_OX);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_OX);

    return status;
}
float nECU_OX_GetValue(void) // returns pointer to resulting data
{
    return OX.sensor.output;
}

/* GPIO inputs */
bool nECU_DigitalInput_Start(nECU_DigiInput_ID ID)
{
    if (ID >= DigiInput_ID_MAX)
        return true;

    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_DigiInput_CRANKING + ID))
    {
        Input_List[ID].GPIO_Pin = Input_Pin_List[ID];
        Input_List[ID].GPIOx = Input_Port_List[ID];
        Input_List[ID].State = GPIO_PIN_RESET;

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_DigiInput_CRANKING + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_DigiInput_CRANKING + ID) && status == false)
    {
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_DigiInput_CRANKING + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_DigiInput_CRANKING + ID);

    return status;
}
bool nECU_DigitalInput_Stop(nECU_DigiInput_ID ID)
{
    if (ID >= DigiInput_ID_MAX)
        return true;

    bool status = false;

    if (nECU_FlowControl_Working_Check(D_DigiInput_CRANKING + ID) && status == false)
    {
        Input_List[ID].GPIOx = NULL;
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_DigiInput_CRANKING + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_DigiInput_CRANKING + ID);

    return status;
}
void nECU_DigitalInput_Routine(nECU_DigiInput_ID ID)
{
    if (ID >= DigiInput_ID_MAX)
        return;

    if (!nECU_FlowControl_Working_Check(D_DigiInput_CRANKING + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_DigiInput_CRANKING + ID);
        return; // Break
    }

    Input_List[ID].State = HAL_GPIO_ReadPin(Input_List[ID].GPIOx, Input_List[ID].GPIO_Pin);

    nECU_Debug_ProgramBlockData_Update(D_DigiInput_CRANKING + ID);
}

bool nECU_DigitalInput_getValue(nECU_DigiInput_ID ID)
{
    if (ID >= DigiInput_ID_MAX)
        return false;

    if (!nECU_FlowControl_Working_Check(D_DigiInput_CRANKING + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_DigiInput_CRANKING + ID);
        return false; // Break
    }

    return (bool)Input_List[ID].State;
}

/* Immobilizer */
bool none = true;
bool *nECU_Immo_getPointer(void) // returns pointer to immobilizer valid
{
    return &none;
}
