/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

static nECU_Debug dbg_data = {0};

/* Debug main functions */
bool nECU_Debug_Start(void) // starts up debugging functions
{
    bool status = false;

    nECU_Debug_ProgramBlock_Init();

    if (!nECU_FlowControl_Initialize_Check(D_Debug))
    {
        status |= nECU_Debug_Init_Struct();
        status |= nECU_Debug_Init_Que();
        status |= nECU_InputAnalog_ADC1_Start(ADC1_MCUTemp_ID);
        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_Debug);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_Debug) && status == false)
    {
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_Debug);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Debug);
    }

    return status;
}
static bool nECU_Debug_Init_Struct(void) // set values to variables in structure
{
    bool status = false;
    // EGT sensors
    for (uint8_t current_ID = 0; current_ID < EGT_ID_MAX; current_ID++) // Do for all EGT sensors
    {
        /* Device temperature */
        if (nECU_EGT_getPointer_TemperatureIC(current_ID)) // Check if pointer exists
        {
            dbg_data.device_temperature.EGT_IC[current_ID] = nECU_EGT_getPointer_TemperatureIC(current_ID);
        }
        else
        {
            status |= nECU_FlowControl_Error_Do(D_Debug);
        }
        /* EGT temperature (thermocuple temperature) */
        if (nECU_EGT_getPointer_Temperature(current_ID)) // Check if pointer exists
        {
            dbg_data.egt_temperature.EGT_IC[current_ID] = nECU_EGT_getPointer_Temperature(current_ID);
        }
        else
        {
            status |= nECU_FlowControl_Error_Do(D_Debug);
        }
        /* EGT communication */
        if (nECU_EGT_getPointer_Error(current_ID)) // Check if pointer exists
        {
            dbg_data.egt_communication.EGT_IC[current_ID] = nECU_EGT_getPointer_Error(current_ID);
        }
        else
        {
            status |= nECU_FlowControl_Error_Do(D_Debug);
        }
    }

    return status;
}
void nECU_Debug_Periodic(void) // checks states of variables
{
    if (!nECU_FlowControl_Working_Check(D_Debug))
    {
        nECU_FlowControl_Error_Do(D_Debug);
        return;
    }
    nECU_InputAnalog_ADC1_Routine(ADC1_MCUTemp_ID);
    dbg_data.device_temperature.MCU = nECU_FloatToInt(nECU_InputAnalog_ADC1_getValue(ADC1_MCUTemp_ID), 16);

    nECU_Debug_IntTemp_Check(&(dbg_data.device_temperature));
    // nECU_Debug_EGTTemp_Check(&(dbg_data.egt_temperature));
    // nECU_Debug_EGTsensor_error(&(dbg_data.egt_communication));
    // nECU_Debug_CAN_Check();
    // nECU_Debug_SPI_Check();
    nECU_Debug_ProgramBlockData_Update(D_Debug);
}

/* Check states routines */
static void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst) // check for errors of device temperature
{
    if (nECU_FlowControl_Working_Check(D_ANALOG_MCUTemp)) // do after startup delay is done
    {
        if (nECU_Debug_IntTemp_CheckSingle(&(inst->MCU))) // check main IC
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, (float)(inst->MCU), nECU_ERROR_DEVICE_TEMP_MCU_ID);
        }
    }
    for (uint8_t i = 0; i < 4; i++) // check all EGT devices
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, (float)*(inst->EGT_IC[i]), nECU_ERROR_DEVICE_TEMP_EGT1_ID + i);
        }
    }
}
static bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature) // checks if passed temperature is in defined bounds
{
    /* true => out of bounds, false => no errors */
    if ((*temperature) > DEVICE_TEMPERATURE_MAX)
        return true;

    else if ((*temperature) < DEVICE_TEMPERATURE_MIN)
        return true;

    return false;
}
static void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst) // check if TCs did not exceed fault value
{
    for (uint8_t i = 0; i < 4; i++)
        if (nECU_Debug_EGTTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, *(inst->EGT_IC[i]), nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
}
static bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature) // checks if passed temperature is in defined bound
{
    if (*temperature > TC_TEMPERATURE_MAX)
        return true;

    return false;
}
static void nECU_Debug_EGTsensor_error(nECU_Debug_EGT_Comm *inst) // check EGT ICs for error flags
{
    if (BENCH_MODE == true)
        return;

    for (uint8_t i = 0; i < 4; i++)
    {
        if (inst->EGT_IC[i]) // if error detected set message
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, (float)*inst->EGT_IC[i], nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
static void nECU_Debug_CAN_Check(void) // checks if CAN have any error pending
{
    if (!nECU_FlowControl_Working_Check(D_CAN_TX) || !nECU_FlowControl_Working_Check(D_CAN_RX)) // Check if RX or TX is working
        if (nECU_CAN_GetError())
        {
            uint32_t error = HAL_CAN_GetError(&hcan1);
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_CAN_ID);
        }
}
static void nECU_Debug_SPI_Check(void) // checks if SPI have any error pending
{
    SPI_HandleTypeDef *hspi = nECU_SPI_getPointer(SPI_EGT_ID);
    if (HAL_SPI_GetState(hspi) == HAL_SPI_STATE_ERROR)
    {
        uint32_t error = HAL_SPI_GetError(hspi);
        if (error > HAL_SPI_ERROR_NONE)
        {
            hspi->ErrorCode = HAL_SPI_ERROR_NONE; // resets error
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_SPI_ID);
        }
    }
}

/* Functions to call directly from other files */
void nECU_Debug_EGTSPIcomm_error(EGT_Sensor_ID ID) // to be called when SPI error occurs
{
    nECU_Debug_error_mesage temporary;
    nECU_Debug_Message_Set(&temporary, (float)HAL_GetTick(), nECU_ERROR_EGT_SPI_EGT1_ID + ID);
}
void nECU_Debug_FLASH_error(nECU_Flash_Error_ID ID, bool write_read) // indicate error from flash functions
{
    // write_read ==  true => writing, false => reading
    nECU_Debug_error_mesage temporary;
    nECU_Error_ID id;
    switch (ID)
    {
    case nECU_FLASH_ERROR_SPEED:
        id = nECU_ERROR_FLASH_SPEED_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_USER:
        id = nECU_ERROR_FLASH_USER_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_DBGQUE:
        id = nECU_ERROR_FLASH_DEBUG_QUE_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_ERASE:
        id = nECU_ERROR_FLASH_ERASE_ID;
        break;

    default:
        break;
    }
    nECU_Debug_Message_Set(&temporary, (float)HAL_GetTick(), id);
}

/* Debug que and messages */
static bool nECU_Debug_Init_Que(void) // initializes que
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_Debug_Que))
    {
        dbg_data.error_que.counter.preset = sizeof(dbg_data.error_que.messages) / sizeof(nECU_Debug_error_mesage); // calculate length of que
        dbg_data.error_que.counter.value = 0;
        dbg_data.error_que.message_count = 0;
        for (uint16_t que_index = 0; que_index < dbg_data.error_que.counter.preset; que_index++)
        {
            nECU_Debug_Message_Init(&(dbg_data.error_que.messages[que_index])); // clear each
        }
        status |= nECU_Flash_DebugQue_read(&(dbg_data.error_que));
        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_Debug_Que);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_Debug_Que) && status == false)
    {
        status |= !nECU_FlowControl_Working_Do(D_Debug_Que);
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Debug_Que);
    }

    return status;
}
static void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message) // add message to debug que
{
    if (!nECU_FlowControl_Working_Check(D_Debug_Que))
    {
        nECU_FlowControl_Error_Do(D_Debug_Que);
        return;
    }
    if (dbg_data.error_que.counter.value == dbg_data.error_que.counter.preset) // check if reached maximum value
    {
        dbg_data.error_que.counter.value = 0;
    }
    if (dbg_data.error_que.message_count >= dbg_data.error_que.counter.preset) // cap the maximum number of messages
    {
        dbg_data.error_que.message_count = dbg_data.error_que.counter.preset;
    }
    memcpy(&(dbg_data.error_que.messages[dbg_data.error_que.counter.value]), message, sizeof(nECU_Debug_error_mesage)); // copy to que
    dbg_data.error_que.counter.value++;
    dbg_data.error_que.message_count++;
}
void nECU_Debug_Que_Read(nECU_Debug_error_mesage *message) // read newest message from debug que
{
    if (!nECU_FlowControl_Working_Check(D_Debug_Que))
    {
        nECU_FlowControl_Error_Do(D_Debug_Que);
        return;
    }
    if (dbg_data.error_que.message_count == 0) // break if no messages in que
    {
        return;
    }

    memcpy(message, &(dbg_data.error_que.messages[dbg_data.error_que.counter.value]), sizeof(nECU_Debug_error_mesage)); // read last message

    nECU_Debug_Message_Init(&(dbg_data.error_que.messages[dbg_data.error_que.counter.value])); // clear que position

    dbg_data.error_que.counter.value--;
    dbg_data.error_que.message_count--;
}
static void nECU_Debug_Message_Init(nECU_Debug_error_mesage *inst) // zeros value inside of structure
{
    inst->error_flag = false;
    inst->ID = 0;
    inst->value_at_flag = 0.0;
}
void nECU_Debug_Message_Set(nECU_Debug_error_mesage *inst, float value, nECU_Error_ID ID) // sets error values
{
    inst->error_flag = true;
    inst->value_at_flag = value;
    inst->ID = ID;
    nECU_Debug_Que_Write(inst);
    int8_t after_dot = ((int)round(value * 100) % 100);
    if (after_dot < 0)
        after_dot = -after_dot;

    printf("New error written to Debug_Que. Value at flag %d.%d, ID: %d\n\r", (int)round(inst->value_at_flag), after_dot, inst->ID);
}
