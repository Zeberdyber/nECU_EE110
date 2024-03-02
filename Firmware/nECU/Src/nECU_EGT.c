/**
 ******************************************************************************
 * @file    nECU_EGT.c
 * @brief   This file provides code for eghaust gas temperature sensors
 *          with its MAX31855 converters.
 ******************************************************************************
 */

#include "nECU_EGT.h"

/* EGT variables */
nECU_EGT EGT_variables;

/* interface functions */
MAX31855 *EGT_IdentifyID(EGT_Sensor_ID ID) // returns pointer to appropriete structure
{
    switch (ID)
    {
    case EGT_CYL1:
        return &EGT_variables.TC1;
        break;
    case EGT_CYL2:
        return &EGT_variables.TC2;
        break;
    case EGT_CYL3:
        return &EGT_variables.TC3;
        break;
    case EGT_CYL4:
        return &EGT_variables.TC4;
        break;

    default:
        break;
    }
    return &EGT_variables.TC1; // default behaviour
}
uint16_t *EGT_GetTemperaturePointer(EGT_Sensor_ID ID) // get function that returns pointer to output data of sensor, ready for can transmission
{
    MAX31855 *inst = EGT_IdentifyID(ID);
    return &inst->EGT_Temperature;
}
uint16_t *EGT_GetTemperatureInternalPointer(EGT_Sensor_ID ID) // get function that returns pointer to internal temperature data of sensor
{
    MAX31855 *inst = EGT_IdentifyID(ID);
    return &inst->IC_Temperature;
}
bool *EGT_GetInitialized(void) // get function to check if code was EGT_Initialized
{
    return &EGT_variables.EGT_Initialized;
}
bool *EGT_GetUpdateOngoing(void) // get function to check if current comunication is ongoing
{
    return &EGT_variables.EGT_CommunicationOngoing;
}
EGT_Error_Code *EGT_GetErrorState(EGT_Sensor_ID ID) // get function returns pointer to error code
{
    MAX31855 *inst = EGT_IdentifyID(ID);
    return &inst->ErrCode;
}

/* EGT functions */
void EGT_Start(void) // initialize all sensors and start communication
{
    MAX31855_Init(&EGT_variables.TC1, &SPI_PERIPHERAL_EGT, T1_CS_GPIO_Port, T1_CS_Pin);
    MAX31855_Init(&EGT_variables.TC2, &SPI_PERIPHERAL_EGT, T2_CS_GPIO_Port, T2_CS_Pin);
    MAX31855_Init(&EGT_variables.TC3, &SPI_PERIPHERAL_EGT, T3_CS_GPIO_Port, T3_CS_Pin);
    MAX31855_Init(&EGT_variables.TC4, &SPI_PERIPHERAL_EGT, T4_CS_GPIO_Port, T4_CS_Pin);
    EGT_variables.EGT_CurrentSensor = 0;
    EGT_variables.EGT_FirstSensor = true;
    EGT_variables.EGT_Initialized = true;
}
void EGT_GetSPIData(bool error) // get data of all sensors
{
    if (EGT_variables.EGT_FirstSensor == true)
    {
        EGT_variables.EGT_CurrentSensor = 1; // after first was done go to second sensor
        EGT_variables.EGT_FirstSensor = false;
        EGT_variables.EGT_CommunicationOngoing = true;
    }
    else
    {
        HAL_GPIO_WritePin(EGT_variables.EGT_CurrentObj->CS_pin.GPIOx, EGT_variables.EGT_CurrentObj->CS_pin.GPIO_Pin, SET);
        EGT_variables.EGT_CurrentObj->data_Pending++;
        EGT_variables.EGT_CurrentSensor++; // go to next sensor
    }
    if (EGT_variables.EGT_CurrentSensor > 4) // cycle break if all sensors done
    {
        EGT_variables.EGT_CurrentSensor = 0;
        EGT_variables.EGT_CommunicationOngoing = false;
        return;
    }

    switch (EGT_variables.EGT_CurrentSensor) // assign next MAX31855
    {
    case EGT_CYL1:
        EGT_variables.EGT_CurrentObj = &EGT_variables.TC1;
        break;
    case EGT_CYL2:
        EGT_variables.EGT_CurrentObj = &EGT_variables.TC2;
        break;
    case EGT_CYL3:
        EGT_variables.EGT_CurrentObj = &EGT_variables.TC3;
        break;
    case EGT_CYL4:
        EGT_variables.EGT_CurrentObj = &EGT_variables.TC4;
        break;

    default:
        return;
        break;
    }
    nECU_SPI_Rx_IT_Start(EGT_variables.EGT_CurrentObj->CS_pin.GPIOx, &EGT_variables.EGT_CurrentObj->CS_pin.GPIO_Pin, EGT_variables.EGT_CurrentObj->hspi, (uint8_t *)EGT_variables.EGT_CurrentObj->in_buffer, 4); // start reciving data
}
void EGT_ConvertAll(void) // convert data if pending
{
    if (EGT_variables.TC1.data_Pending > 0)
    {
        MAX31855_ConvertData(&EGT_variables.TC1);
        EGT_TemperatureTo10bit(&EGT_variables.TC1);
    }
    if (EGT_variables.TC2.data_Pending > 0)
    {
        MAX31855_ConvertData(&EGT_variables.TC2);
        EGT_TemperatureTo10bit(&EGT_variables.TC2);
    }
    if (EGT_variables.TC3.data_Pending > 0)
    {
        MAX31855_ConvertData(&EGT_variables.TC3);
        EGT_TemperatureTo10bit(&EGT_variables.TC3);
    }
    if (EGT_variables.TC4.data_Pending > 0)
    {
        MAX31855_ConvertData(&EGT_variables.TC4);
        EGT_TemperatureTo10bit(&EGT_variables.TC4);
    }
}
void EGT_TemperatureTo10bit(MAX31855 *inst) // function to convert temperature value to 10bit number for CAN transmission
{
    float Input = inst->TcTemp;
    for (uint8_t i = 0; i < EGT_DECIMAL_POINT; i++) // extend to desired decimal point
    {
        Input *= 10;
    }
    Input -= EGT_NEGATIVE_OFFSET;
    if (Input < 0.0 || Input > 1023.0) // if out of 10bit bound zero out
    {
        Input = 0.0;
    }
    inst->EGT_Temperature = (uint16_t)Input;
}

void EGT_PeriodicEventHP(void) // high priority periodic event, launched from timer interrupt
{
    if (EGT_variables.EGT_CurrentSensor == 0)
    {
        EGT_variables.EGT_FirstSensor = true;
        EGT_GetSPIData(false);
    }
    if (EGT_variables.TC1.data_Pending > EGT_MAXIMUM_PENDING_COUNT || EGT_variables.TC2.data_Pending > EGT_MAXIMUM_PENDING_COUNT || EGT_variables.TC3.data_Pending > EGT_MAXIMUM_PENDING_COUNT || EGT_variables.TC4.data_Pending > EGT_MAXIMUM_PENDING_COUNT)
    {
        EGT_PeriodicEventLP();
    }
}
void EGT_PeriodicEventLP(void) // low priority periodic event, launched fsrom regular main loop
{
    if (EGT_variables.EGT_Initialized == false)
    {
        EGT_Start();
    }
    EGT_ConvertAll();
}

void MAX31855_Init(MAX31855 *inst, SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) // First initialization
{
    inst->OC_Fault = false;
    inst->SCG_Fault = false;
    inst->SCV_Fault = false;
    inst->Data_Error = false;
    inst->InternalTemp = 0;
    inst->TcTemp = 0;

    inst->hspi = hspi;
    inst->CS_pin.GPIOx = GPIOx;
    inst->CS_pin.GPIO_Pin = GPIO_Pin;
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, SET);
}
void MAX31855_collectError(MAX31855 *inst) // get current error value
{
    if (inst->SCG_Fault || inst->SCV_Fault)
    {
        inst->ErrCode = EGT_ERROR_SC;
        return;
    }
    else if (inst->OC_Fault)
    {
        inst->ErrCode = EGT_ERROR_OC;
        return;
    }
    else if (inst->Data_Error)
    {
        inst->ErrCode = EGT_ERROR_DATA;
        return;
    }
}
void MAX31855_UpdateSimple(MAX31855 *inst) // Recive data over SPI and convert it into struct, dont use while in DMA mode
{
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, RESET);
    // HAL_Delay(1);
    HAL_SPI_Receive_IT(inst->hspi, (uint8_t *)inst->in_buffer, sizeof(inst->in_buffer));
    inst->data_Pending++;
    HAL_Delay(1); // delay for callback
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, SET);
    MAX31855_ConvertData(inst);
}
void MAX31855_ConvertData(MAX31855 *inst) // For internal use bit decoding and data interpretation
{
    inst->OC_Fault = (inst->in_buffer[3] >> 0) & 0x01;
    inst->SCG_Fault = (inst->in_buffer[3] >> 1) & 0x01;
    inst->SCV_Fault = (inst->in_buffer[3] >> 2) & 0x01;
    inst->Data_Error = (inst->in_buffer[3] >> 3) || (inst->in_buffer[1] >> 7) & 0x01;
    inst->InternalTemp = 99;
    inst->TcTemp = 1300;

    if (inst->Data_Error == false)
    {
        inst->TcTemp = ((inst->in_buffer[0] << 6) | (inst->in_buffer[1] >> 2)) * 0.25;
        if (inst->in_buffer[0] & 0x80) // negative sign
            inst->TcTemp = -inst->TcTemp;

        inst->InternalTemp = ((inst->in_buffer[2] << 4) | (inst->in_buffer[3] >> 4)) * 0.0625;
        if (inst->in_buffer[2] & 0x80) // negative sign
            inst->InternalTemp = -inst->InternalTemp;

        inst->IC_Temperature = inst->InternalTemp; // float to int16_t
    }

    inst->data_Pending = 0;
    MAX31855_collectError(inst);
}
