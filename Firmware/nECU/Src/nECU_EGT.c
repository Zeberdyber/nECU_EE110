/**
 ******************************************************************************
 * @file    nECU_EGT.c
 * @brief   This file provides code for eghaust gas temperature sensors
 *          with its MAX31855 converters.
 ******************************************************************************
 */

#include "nECU_EGT.h"

/* EGT variables */
nECU_EGT EGT_variables = {0};

// Look up table
static GPIO_TypeDef const *const EGT_GPIO_Port_List[EGT_ID_MAX] = {
    [EGT1_ID] = T1_CS_GPIO_Port,
    [EGT2_ID] = T2_CS_GPIO_Port,
    [EGT3_ID] = T3_CS_GPIO_Port,
    [EGT4_ID] = T4_CS_GPIO_Port,
};
static GPIO_TypeDef const *const EGT_GPIO_Pin_List[EGT_ID_MAX] = {
    [EGT1_ID] = T1_CS_Pin,
    [EGT2_ID] = T2_CS_Pin,
    [EGT3_ID] = T3_CS_Pin,
    [EGT4_ID] = T4_CS_Pin,
};

/* interface functions */
uint16_t *EGT_GetTemperaturePointer(EGT_Sensor_ID ID) // get function that returns pointer to output data of sensor, ready for can transmission
{
    if (ID >= EGT_ID_MAX)
    {
        nECU_FlowControl_Error_Do(D_EGT1);
        nECU_FlowControl_Error_Do(D_EGT2);
        nECU_FlowControl_Error_Do(D_EGT3);
        nECU_FlowControl_Error_Do(D_EGT4);
        return NULL;
    }

    return EGT_variables.TC[ID].EGT_Temperature;
}
int16_t *EGT_GetTemperatureInternalPointer(EGT_Sensor_ID ID) // get function that returns pointer to internal temperature data of sensor
{
    if (ID >= EGT_ID_MAX)
    {
        nECU_FlowControl_Error_Do(D_EGT1);
        nECU_FlowControl_Error_Do(D_EGT2);
        nECU_FlowControl_Error_Do(D_EGT3);
        nECU_FlowControl_Error_Do(D_EGT4);
        return NULL;
    }

    return EGT_variables.TC[ID].IC_Temperature;
}
EGT_Error_Code *EGT_GetErrorState(EGT_Sensor_ID ID) // get function returns pointer to error code
{
    if (ID >= EGT_ID_MAX)
    {
        nECU_FlowControl_Error_Do(D_EGT1);
        nECU_FlowControl_Error_Do(D_EGT2);
        nECU_FlowControl_Error_Do(D_EGT3);
        nECU_FlowControl_Error_Do(D_EGT4);
        return NULL;
    }

    return EGT_variables.TC[ID].ErrCode;
}

/* EGT functions */
bool EGT_Start(void) // initialize all sensors and start communication
{
    bool status = false;
    EGT_variables.EGT_CurrentSensor = 0;

    // Delay needed as EGT ICs may take some time to startup
    uint32_t delay = EGT_STARTUP_DELAY;
    nECU_Delay_Set(&(EGT_variables.startup_Delay), &delay);
    nECU_Delay_Start(&(EGT_variables.startup_Delay));

    for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++) // initialize all egt sensors
    {
        bool current_status = false;
        if (!nECU_FlowControl_Initialize_Check(D_EGT1 + current_ID))
        {
            current_status |= MAX31855_Init(&EGT_variables.TC[current_ID], &SPI_PERIPHERAL_EGT, EGT_GPIO_Port_List[current_ID], EGT_GPIO_Pin_List[current_ID]);
            if (!current_status)
            {
                current_status |= !nECU_FlowControl_Initialize_Check(D_EGT1 + current_ID);
            }
        }
        if (!nECU_FlowControl_Working_Check(D_EGT1 + current_ID))
        {
            current_status |= !nECU_FlowControl_Working_Do(D_EGT1 + current_ID);
        }
        if (current_status)
        {
            nECU_FlowControl_Error_Do(D_EGT1 + current_ID);
        }
        status |= current_status;
    }

    return status;
}
void EGT_Stop(void) // stop the routines
{
    UNUSED(NULL);
}

void EGT_Routine(void) // periodic function to be called every main loop execution
{
    nECU_Delay_Update(&(EGT_variables.startup_Delay));

    for (EGT_Sensor_ID Current_ID = 0; Current_ID < EGT_ID_MAX; Current_ID++) // Check if all are working
    {
        if (!nECU_FlowControl_Working_Check(D_EGT1 + Current_ID))
        {
            nECU_FlowControl_Error_Do(D_EGT1 + Current_ID);
            return;
        }
    }
    if (EGT_variables.startup_Delay.done == false) // Break if still in booting
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
        else if (EGT_variables.EGT_CurrentSensor >= 4) // do if all sensors were done
        {
            EGT_ConvertAll();
            EGT_variables.EGT_CurrentSensor = 0;
            EGT_variables.updatePending = false;
        }
    }
}
static void EGT_ConvertAll(void) // convert data if pending
{
    for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++)
    {
        if (EGT_variables.TC[current_ID].data_Pending > 0)
        {
            MAX31855_ConvertData(&EGT_variables.TC[current_ID]);
            EGT_TemperatureTo10bit(&EGT_variables.TC[current_ID]);
        }
    }
}
static void EGT_TemperatureTo10bit(MAX31855 *inst) // function to convert temperature value to 10bit number for CAN transmission
{
    float Input = inst->TcTemp;
    for (uint8_t i = 0; i <= EGT_DECIMAL_POINT; i++) // extend to desired decimal point
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

MAX31855 *EGT_SPI_getNext(uint8_t sensorNumber) // returns pointer to correct IC
{
    sensorNumber = sensorNumber;
    switch (EGT_variables.EGT_CurrentSensor) // assign next MAX31855
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
    default:
        break;
    }

    return NULL;
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
    HAL_GPIO_WritePin(EGT_variables.EGT_CurrentObj->CS_pin.GPIOx, EGT_variables.EGT_CurrentObj->CS_pin.GPIO_Pin, GPIO_PIN_SET);

    if (error == true) // if error
    {
        EGT_variables.EGT_CurrentObj->comm_fail++;
        if (EGT_variables.EGT_CurrentObj->comm_fail > EGT_COMM_FAIL_MAX) // if fail threshold reached
        {
            nECU_Debug_EGTSPIcomm_error(EGT_variables.EGT_CurrentSensor);
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
    EGT_variables.EGT_CurrentSensor = 0; // Start from the top
}

bool MAX31855_Init(MAX31855 *inst, SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) // First initialization
{
    bool status = false;

    inst->OC_Fault = false;
    inst->SCG_Fault = false;
    inst->SCV_Fault = false;
    inst->Data_Error = false;
    inst->InternalTemp = 0;
    inst->TcTemp = 0;

    inst->hspi = hspi;
    inst->CS_pin.GPIOx = GPIOx;
    inst->CS_pin.GPIO_Pin = GPIO_Pin;
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_SET);

    return status;
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
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_RESET);
    // HAL_Delay(1);
    HAL_SPI_Receive_IT(inst->hspi, (uint8_t *)inst->in_buffer, sizeof(inst->in_buffer));
    inst->data_Pending++;
    HAL_Delay(1); // delay for callback
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_SET);
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

        inst->IC_Temperature = inst->InternalTemp * INTERNAL_TEMP_MULTIPLIER; // float to int16_t
    }

    inst->data_Pending = 0;
    MAX31855_collectError(inst);
}
