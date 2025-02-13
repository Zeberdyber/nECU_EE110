/**
 ******************************************************************************
 * @file    nECU_EGT.c
 * @brief   This file provides code for eghaust gas temperature sensors
 *          with its MAX31855 converters.
 ******************************************************************************
 */

#include "nECU_EGT.h"

/* EGT variables */
nECU_EGT EGT_data = {0};

// Look up table
static GPIO_TypeDef *const EGT_GPIO_Port_List[EGT_ID_MAX] = {
    [EGT1_ID] = T1_CS_GPIO_Port,
    [EGT2_ID] = T2_CS_GPIO_Port,
    [EGT3_ID] = T3_CS_GPIO_Port,
    [EGT4_ID] = T4_CS_GPIO_Port,
};
static const uint16_t EGT_GPIO_Pin_List[EGT_ID_MAX] = {
    [EGT1_ID] = T1_CS_Pin,
    [EGT2_ID] = T2_CS_Pin,
    [EGT3_ID] = T3_CS_Pin,
    [EGT4_ID] = T4_CS_Pin,
};

/* interface functions */
uint16_t *nECU_EGT_getPointer_Temperature(EGT_Sensor_ID ID) // get function that returns pointer to output data of sensor, ready for can transmission
{
    if (ID >= EGT_ID_MAX)
        return NULL;

    return &EGT_data.TC[ID].EGT_Temperature;
}
int16_t *nECU_EGT_getPointer_TemperatureIC(EGT_Sensor_ID ID) // get function that returns pointer to internal temperature data of sensor
{
    if (ID >= EGT_ID_MAX)
        return NULL;

    return &EGT_data.TC[ID].IC_Temperature;
}
EGT_Error_Code *nECU_EGT_getPointer_Error(EGT_Sensor_ID ID) // get function returns pointer to error code
{
    if (ID >= EGT_ID_MAX)
        return NULL;

    return &EGT_data.TC[ID].ErrCode;
}

/* EGT functions */
bool nECU_EGT_Start(void) // initialize all sensors and start communication
{
    bool status = false;
    EGT_data.currentSensor = 0;

    // Delay needed as EGT ICs may take some time to startup
    nECU_Delay_Set(&(EGT_data.startup), EGT_STARTUP_DELAY);
    nECU_Delay_Start(&(EGT_data.startup));

    for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++) // initialize all egt sensors
    {
        status |= nECU_EGT_Start_Single(current_ID);
    }

    return status;
}
bool nECU_EGT_Stop(void) // stop the routines
{
    bool status = false;
    for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++) // stop all egt sensors
    {
        status |= nECU_EGT_Stop_Single(current_ID);
    }

    return status;
}
void nECU_EGT_Routine(void) // periodic function to be called every main loop execution
{
    if (EGT_data.startup.done == false) // Break if still in booting
    {
        nECU_Delay_Update(&(EGT_data.startup));
        return;
    }
    // MAX31855_UpdateSimple(&EGT_data.TC[0], &hspi1);
    // MAX31855_UpdateSimple(&EGT_data.TC[1], &hspi1);
    // MAX31855_UpdateSimple(&EGT_data.TC[2], &hspi1);
    // MAX31855_UpdateSimple(&EGT_data.TC[3], &hspi1);

    // // do the update for each sensor
    // if (EGT_data.currentSensor == EGT1_ID) // begin the transmission
    //     nECU_EGT_Callback();
    // else if (EGT_data.currentSensor >= EGT_ID_MAX) // do if all sensors were done
    // {
    //     for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++)
    //     {
    //         if (nECU_FlowControl_Working_Check(D_EGT1 + current_ID)) // Do for working sensors
    //         {
    //             MAX31855_ConvertData(&EGT_data.TC[current_ID]);
    //         }
    //     }
    //     EGT_data.currentSensor = 0;
    // }
}
void nECU_EGT_RequestUpdate(void) // indicate that update is needed
{
    EGT_data.currentSensor = 0; // Start from the top
}

void nECU_EGT_Callback(void) // callback from SPI_TX end callback
{

    EGT_data.currentSensor++;
    if (EGT_data.currentSensor >= EGT_ID_MAX)
        return;
    while (!nECU_FlowControl_Working_Check(D_EGT1 + (EGT_data.currentSensor))) // find next working sensor
    {
        EGT_data.currentSensor++;
        if (EGT_data.currentSensor >= EGT_ID_MAX)
            return;
    }
    nECU_SPI_Rx_IT_Start(&(EGT_data.TC[EGT_data.currentSensor].CS_pin), SPI_EGT_ID, (uint8_t *)EGT_data.TC[EGT_data.currentSensor].in_buffer, 4); // start reciving data
}
void nECU_EGT_Error_Callback(void) // Callback after SPI communication fail
{
    if (EGT_data.TC[EGT_data.currentSensor].comm_fail > EGT_COMM_FAIL_MAX) // if fail threshold reached
    {
        nECU_Debug_EGTSPIcomm_error(EGT_data.currentSensor);
        EGT_data.TC[EGT_data.currentSensor].comm_fail = 0;
    }
    nECU_EGT_Callback();
}

static bool nECU_EGT_Start_Single(EGT_Sensor_ID ID) // Perform start for single sensor
{
    if (ID >= EGT_ID_MAX)
        return true;

    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_EGT1 + ID))
    {
        status |= MAX31855_Init(&EGT_data.TC[ID], EGT_GPIO_Port_List[ID], EGT_GPIO_Pin_List[ID]);
        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_EGT1 + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_EGT1 + ID) && status == false)
        status |= !nECU_FlowControl_Working_Do(D_EGT1 + ID);

    if (status)
        nECU_FlowControl_Error_Do(D_EGT1 + ID);

    return status;
}
static bool nECU_EGT_Stop_Single(EGT_Sensor_ID ID) // Perform stop for single sensor
{
    if (ID >= EGT_ID_MAX)
        return true;

    bool status = false;
    if (nECU_FlowControl_Working_Check(D_EGT1 + ID) && status == false)
    {
        status |= !nECU_FlowControl_Stop_Do(D_EGT1 + ID);
    }

    if (status)
    {
        nECU_FlowControl_Error_Do(D_EGT1 + ID);
    }
    return status;
}

static bool MAX31855_Init(MAX31855 *inst, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) // First initialization
{
    if (inst == NULL || GPIOx == NULL) // Break if pointers do not exist
        return true;

    bool status = false;

    inst->OC_Fault = false;
    inst->SCG_Fault = false;
    inst->SCV_Fault = false;
    inst->Data_Error = false;
    inst->InternalTemp = 0;
    inst->TcTemp = 0;

    inst->CS_pin.GPIOx = GPIOx;
    inst->CS_pin.GPIO_Pin = GPIO_Pin;
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_SET);

    return status;
}
static void MAX31855_collectError(MAX31855 *inst) // get current error value
{
    if (inst == NULL) // Break if pointers do not exist
        return;

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
static void MAX31855_UpdateSimple(MAX31855 *inst, SPI_HandleTypeDef *hspi) // Recive data over SPI and convert it into struct, dont use while in DMA mode
{
    if (inst == NULL || hspi == NULL) // Break if pointers do not exist
        return;

    HAL_StatusTypeDef status = HAL_OK;
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_RESET);
    status = HAL_SPI_Receive(hspi, (uint8_t *)inst->in_buffer, sizeof(inst->in_buffer), 100);
    HAL_GPIO_WritePin(inst->CS_pin.GPIOx, inst->CS_pin.GPIO_Pin, GPIO_PIN_SET);
    if (status == HAL_OK)
        MAX31855_ConvertData(inst);
    else
        return;
}
static void MAX31855_ConvertData(MAX31855 *inst) // For internal use bit decoding and data interpretation
{
    if (inst == NULL) // Break if pointers do not exist
        return;

    inst->OC_Fault = (inst->in_buffer[3] >> 0) & 0x01;
    inst->SCG_Fault = (inst->in_buffer[3] >> 1) & 0x01;
    inst->SCV_Fault = (inst->in_buffer[3] >> 2) & 0x01;
    inst->Data_Error = (inst->in_buffer[1] >> 7) & 0x01;
    inst->InternalTemp = 130;
    inst->TcTemp = 1300;

    if (!(inst->Data_Error) || BENCH_MODE) // ignore errors in bench mode
    {
        inst->TcTemp = ((inst->in_buffer[0] << 6) | (inst->in_buffer[1] >> 2)) * 0.25;
        if (inst->in_buffer[0] & 0x80) // negative sign
            inst->TcTemp = -inst->TcTemp;

        inst->InternalTemp = ((inst->in_buffer[2] << 4) | (inst->in_buffer[3] >> 4)) * 0.0625;
        if (inst->in_buffer[2] & 0x80) // negative sign
            inst->InternalTemp = -inst->InternalTemp;

        inst->IC_Temperature = inst->InternalTemp; // float to int16_t
        if (inst->IC_Temperature < -100)
        {
            HAL_Delay(1);
        }
    }

    MAX31855_collectError(inst);

    float temp = (inst->TcTemp * (10 ^ (EGT_DECIMAL_POINT + 1))) - EGT_NEGATIVE_OFFSET;
    inst->EGT_Temperature = nECU_FloatToUint(temp, 10); // cut to 10bit
}
