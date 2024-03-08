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

static bool *EGT_initialized; // pointer as delay function is used
static bool EGT_working = false;

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
bool *EGT_GetWorking(void) // get function to check if code was EGT_Initialized
{
    return &EGT_working;
}
EGT_Error_Code *EGT_GetErrorState(EGT_Sensor_ID ID) // get function returns pointer to error code
{
    MAX31855 *inst = EGT_IdentifyID(ID);
    return &inst->ErrCode;
}

/* EGT functions */
void EGT_Init(void) // initialize all sensors and start communication
{
    MAX31855_Init(&EGT_variables.TC1, &SPI_PERIPHERAL_EGT, T1_CS_GPIO_Port, T1_CS_Pin);
    MAX31855_Init(&EGT_variables.TC2, &SPI_PERIPHERAL_EGT, T2_CS_GPIO_Port, T2_CS_Pin);
    MAX31855_Init(&EGT_variables.TC3, &SPI_PERIPHERAL_EGT, T3_CS_GPIO_Port, T3_CS_Pin);
    MAX31855_Init(&EGT_variables.TC4, &SPI_PERIPHERAL_EGT, T4_CS_GPIO_Port, T4_CS_Pin);
    EGT_variables.EGT_CurrentSensor = 0;
    EGT_variables.updatePending = true; // to force first update

    uint32_t delay = EGT_STARTUP_DELAY;
    nECU_Delay_Set(&(EGT_variables.startup_Delay), &delay);
    nECU_Delay_Start(&(EGT_variables.startup_Delay));
    EGT_initialized = nECU_Delay_DoneFlag(&(EGT_variables.startup_Delay));
}
void EGT_Start(void) // start the routines
{
    EGT_Init();
    EGT_working = true;
}
void EGT_Stop(void) // stop the routines
{
    EGT_working = true;
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

void EGT_Periodic(void) // periodic function to be called every main loop execution
{
    if (EGT_working == false || *EGT_initialized == false)
    {
        return;
    }
    if (EGT_variables.updatePending == true)
    {
        // do the update for each sensor
        if (EGT_variables.EGT_CurrentSensor == 0) // begin the transmission
        {
            EGT_SPI_startNext();
        }
        else if (EGT_variables.EGT_CurrentSensor > 4) // do if all sensors were done
        {
            EGT_ConvertAll();
            EGT_variables.EGT_CurrentSensor = 0;
            EGT_variables.updatePending = false;
        }
    }
}
MAX31855 *EGT_SPI_getNext(uint8_t sensorNumber) // returns pointer to correct IC
{
    switch (EGT_variables.EGT_CurrentSensor) // assign next MAX31855
    {
    case EGT_CYL1:
        return &EGT_variables.TC1;
    case EGT_CYL2:
        return &EGT_variables.TC2;
    case EGT_CYL3:
        return &EGT_variables.TC3;
    case EGT_CYL4:
        return &EGT_variables.TC4;
    default:
        return &EGT_variables.TC1;
    }
}
void EGT_SPI_startNext(void) // starts SPI communication for next IC
{
    EGT_variables.EGT_CurrentSensor++;
    EGT_variables.EGT_CurrentObj = EGT_SPI_getNext(EGT_variables.EGT_CurrentSensor);
    nECU_SPI_Rx_IT_Start(EGT_variables.EGT_CurrentObj->CS_pin.GPIOx, &EGT_variables.EGT_CurrentObj->CS_pin.GPIO_Pin, EGT_variables.EGT_CurrentObj->hspi, (uint8_t *)EGT_variables.EGT_CurrentObj->in_buffer, 4); // start reciving data
}
void EGT_SPI_Callback(bool error) // callback from SPI_TX end callback
{
    if (EGT_working == false)
    {
        return; // if this line was called that means there is an error in SPI configuration or direct callback functions
    }

    // Pull pin high to stop transmission
    HAL_GPIO_WritePin(EGT_variables.EGT_CurrentObj->CS_pin.GPIOx, EGT_variables.EGT_CurrentObj->CS_pin.GPIO_Pin, SET);

    if (error = true) // if error
    {
        EGT_variables.EGT_CurrentObj->comm_fail++;
        if (EGT_variables.EGT_CurrentObj->comm_fail > EGT_COMM_FAIL_MAX) // if fail threshold reached
        {
            nECU_Debug_EGTcomm_error(EGT_variables.EGT_CurrentSensor);
            EGT_variables.EGT_CurrentObj->comm_fail = 0;
        }
    }
    else
    {
        EGT_variables.EGT_CurrentObj->data_Pending++; // indicate that data was recived and can be used for the update
    }

    if (EGT_variables.EGT_CurrentSensor < 4) // if not done go to next sensor
    {
        EGT_SPI_startNext();
    }
}
void EGT_RequestUpdate(void) // indicate that update is needed
{
    EGT_variables.updatePending = true;
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
