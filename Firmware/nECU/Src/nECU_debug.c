/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

static nECU_Debug dbg_data;
static nECU_LoopCounter main_loop;

// Program status control
nECU_ProgramBlockData *Debug_Status_List[24]; // array of pointers to all nECU_ProgramBlockData

nECU_ProgramBlockData D_ADC1, D_ADC2, D_ADC3;                                                   // nECU_adc.c
nECU_ProgramBlockData D_Button_Red, D_Button_Orange, D_Button_Green;                            // nECU_button.c
nECU_ProgramBlockData D_CAN_TX, D_CAN_RX;                                                       // nECU_can.c
nECU_ProgramBlockData D_Data_Processing;                                                        // nECU_data_processing.c
nECU_ProgramBlockData D_Debug, D_Debug_Mainloop, D_Debug_Que;                                   // nECU_debug.c
nECU_ProgramBlockData D_Flash;                                                                  // nECU_flash.c
nECU_ProgramBlockData D_F0, D_F1, D_F2;                                                         // nECU_frames.c
nECU_ProgramBlockData D_MAP, D_BackPressure, D_MCU_temperature, D_AdditionalAI, D_Input_Analog; // nECU_Input_Analog.c
nECU_ProgramBlockData D_VSS, D_IGF, D_Input_Frequency;                                          // nECU_Input_Frequency.c

/* Used to track how many times main loop is done between CAN frames */
void nECU_mainLoop_Init(void)
{
    nECU_LoopCounter_Init(&main_loop);
}
void nECU_mainLoop_Update(void)
{
    nECU_LoopCounter_Update(&main_loop);
}
void nECU_mainLoop_Reset(void)
{
    nECU_LoopCounter_Clear(&main_loop);
}
uint32_t *nECU_mainLoop_getValue(void) // returns pointer to current value
{
    return &(main_loop.counter);
}

void nECU_LoopCounter_Init(nECU_LoopCounter *inst) // Initialize structure
{
    inst->counter = 0;
    nECU_TickTrack_Init(&(inst->tracker));
    inst->time = 0;
}
void nECU_LoopCounter_Update(nECU_LoopCounter *inst) // Increment counter, get total time
{
    inst->counter++;
    nECU_TickTrack_Update(&(inst->tracker));
    inst->time += inst->tracker.difference * inst->tracker.convFactor; // add difference in ms
}
void nECU_LoopCounter_Clear(nECU_LoopCounter *inst) // clear value of the counter
{
    inst->counter = 0;
    inst->time = 0;
}

/* Used for simple time tracking */
void nECU_TickTrack_Init(nECU_TickTrack *inst) // initialize structure
{
    inst->previousTick = HAL_GetTick();
    inst->difference = 0;
    inst->convFactor = HAL_GetTickFreq();
}
void nECU_TickTrack_Update(nECU_TickTrack *inst) // callback to get difference
{
    uint64_t tickNow = HAL_GetTick();
    if (tickNow < inst->previousTick) // check if data roll over
    {
        inst->difference = (tickNow + UINT32_MAX) - inst->previousTick;
    }
    else
    {
        inst->difference = tickNow - inst->previousTick;
    }
    inst->previousTick = tickNow;
}

/* Debug main functions */
void nECU_Debug_Start(void) // starts up debugging functions
{
    nECU_Debug_ProgramBlock_Init();
    if (D_Debug.Status = D_BLOCK_STOP)
    {
        nECU_Debug_Init_Struct();
        nECU_Debug_Init_Que();
        nECU_InternalTemp_Init();
        D_Debug.Status |= D_BLOCK_INITIALIZED;
    }

    if (D_Debug.Status & D_BLOCK_INITIALIZED)
    {
        D_Debug.Status |= D_BLOCK_WORKING;
    }
}
void nECU_Debug_Init_Struct(void) // set values to variables in structure
{
    /* Device temperature */
    dbg_data.device_temperature.MCU = nECU_InternalTemp_getTemperature();
    dbg_data.device_temperature.EGT_IC[0] = EGT_GetTemperatureInternalPointer(EGT_CYL1);
    dbg_data.device_temperature.EGT_IC[1] = EGT_GetTemperatureInternalPointer(EGT_CYL2);
    dbg_data.device_temperature.EGT_IC[2] = EGT_GetTemperatureInternalPointer(EGT_CYL3);
    dbg_data.device_temperature.EGT_IC[3] = EGT_GetTemperatureInternalPointer(EGT_CYL4);

    /* EGT temperature (thermocuple temperature) */
    dbg_data.egt_temperature.EGT_IC[0] = EGT_GetTemperaturePointer(EGT_CYL1);
    dbg_data.egt_temperature.EGT_IC[1] = EGT_GetTemperaturePointer(EGT_CYL2);
    dbg_data.egt_temperature.EGT_IC[2] = EGT_GetTemperaturePointer(EGT_CYL3);
    dbg_data.egt_temperature.EGT_IC[3] = EGT_GetTemperaturePointer(EGT_CYL4);

    /* EGT communication */
    dbg_data.egt_communication.EGT_IC[0] = EGT_GetErrorState(EGT_CYL1);
    dbg_data.egt_communication.EGT_IC[1] = EGT_GetErrorState(EGT_CYL2);
    dbg_data.egt_communication.EGT_IC[2] = EGT_GetErrorState(EGT_CYL3);
    dbg_data.egt_communication.EGT_IC[3] = EGT_GetErrorState(EGT_CYL4);
}
void nECU_Debug_Periodic(void) // checks states of variables
{
    if (D_Debug.Status & D_BLOCK_INITIALIZED_WORKING)
    {
        nECU_Debug_IntTemp_Check(&(dbg_data.device_temperature));
        nECU_Debug_EGTTemp_Check(&(dbg_data.egt_temperature));
        nECU_Debug_EGTsensor_error(&(dbg_data.egt_communication));
        nECU_Debug_CAN_Check();
        nECU_Debug_SPI_Check();
    }
}

/* Check states routines */
void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst) // check for errors of device temperature
{
    if (*nECU_InternalTemp_StartupDelay_DoneFlag() == true) // do after startup delay is done
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->MCU)) // check main IC
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, *(inst->MCU), nECU_ERROR_DEVICE_TEMP_MCU_ID);
        }
    }
    for (uint8_t i = 0; i < 4; i++) // check all EGT devices
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, *(inst->EGT_IC[i]), nECU_ERROR_DEVICE_TEMP_EGT1_ID + i);
        }
    }
}
bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature) // checks if passed temperature is in defined bounds
{
    /* true => out of bounds, false => no errors */
    if ((*temperature / INTERNAL_TEMP_MULTIPLIER) > DEVICE_TEMPERATURE_MAX)
    {
        return true;
    }
    else if ((*temperature / INTERNAL_TEMP_MULTIPLIER) < DEVICE_TEMPERATURE_MIN)
    {
        return true;
    }
    return false;
}
void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst) // check if TCs did not exceed fault value
{
    for (uint8_t i = 0; i < 4; i++)
    {
        if (nECU_Debug_EGTTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, *(inst->EGT_IC[i]), nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature) // checks if passed temperature is in defined bound
{
    if (*temperature > TC_TEMPERATURE_MAX)
    {
        return true;
    }
    return false;
}
void nECU_Debug_EGTsensor_error(nECU_Debug_EGT_Comm *inst) // check EGT ICs for error flags
{
    if (BENCH_MODE == true)
    {
        return;
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        if (inst->EGT_IC[i]) // if error detected set message
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, (float)*inst->EGT_IC[i], nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
void nECU_Debug_CAN_Check(void) // checks if CAN have any error pending
{
    if ((D_CAN_TX.Status | D_CAN_RX.Status) & D_BLOCK_WORKING) // Check if RX or TX is working
    {
        uint32_t error = HAL_CAN_GetError(&hcan1);
        if (error > HAL_CAN_ERROR_NONE)
        {
            D_CAN_TX.Status = D_BLOCK_ERROR;
            D_CAN_RX.Status = D_BLOCK_ERROR;

            HAL_CAN_ResetError(&hcan1);
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_CAN_ID);
        }
    }
}
void nECU_Debug_SPI_Check(void) // checks if SPI have any error pending
{
    if (HAL_SPI_GetState(&SPI_PERIPHERAL_EGT) == HAL_SPI_STATE_ERROR)
    {
        uint32_t error = HAL_SPI_GetError(&SPI_PERIPHERAL_EGT);
        if (error > HAL_SPI_ERROR_NONE)
        {
            SPI_PERIPHERAL_EGT.ErrorCode = HAL_SPI_ERROR_NONE; // resets error
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Init(&temp);
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_SPI_ID);
        }
    }
}

/* Functions to call directly from other files */
void nECU_Debug_EGTSPIcomm_error(EGT_Sensor_ID ID) // to be called when SPI error occurs
{
    nECU_Debug_error_mesage temporary;
    nECU_Debug_Message_Init(&temporary);
    nECU_Debug_Message_Set(&temporary, (float)HAL_GetTick(), nECU_ERROR_EGT_SPI_EGT1_ID + ID);
}
void nECU_Debug_FLASH_error(nECU_Flash_Error_ID ID, bool write_read) // indicate error from flash functions
{
    // write_read ==  true => writing, false => reading
    nECU_Debug_error_mesage temporary;
    nECU_Debug_Message_Init(&temporary);
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
void nECU_Debug_Init_Que(void) // initializes que
{
    if (D_Debug_Que.Status = D_BLOCK_STOP)
    {
        dbg_data.error_que.counter.preset = sizeof(dbg_data.error_que.messages) / sizeof(nECU_Debug_error_mesage); // calculate length of que
        dbg_data.error_que.counter.value = 0;
        dbg_data.error_que.message_count = 0;
        for (uint16_t que_index = 0; que_index < dbg_data.error_que.counter.preset; que_index++)
        {
            nECU_Debug_Message_Init(&(dbg_data.error_que.messages[que_index])); // clear each
        }
        D_Debug_Que.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
}
void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message) // add message to debug que
{
    if (D_Debug_Que.Status & D_BLOCK_INITIALIZED_WORKING)
    {
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
    if (D_Debug_Que.Status & D_BLOCK_INITIALIZED_WORKING)
    {
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
void nECU_Debug_Message_Init(nECU_Debug_error_mesage *inst) // zeros value inside of structure
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
}

/* Program Block */
void nECU_Debug_ProgramBlock_Init(void) // Initialize 'ProgramBlock' tracking
{
    // assign ProgramBlockData to list
    Debug_Status_List[0] = &D_ADC1;
    Debug_Status_List[1] = &D_ADC2;
    Debug_Status_List[2] = &D_ADC3;
    Debug_Status_List[3] = &D_Button_Red;
    Debug_Status_List[4] = &D_Button_Orange;
    Debug_Status_List[5] = &D_Button_Green;
    Debug_Status_List[6] = &D_CAN_TX;
    Debug_Status_List[7] = &D_CAN_RX;
    Debug_Status_List[8] = &D_Data_Processing;
    Debug_Status_List[9] = &D_Debug;
    Debug_Status_List[10] = &D_Debug_Mainloop;
    Debug_Status_List[11] = &D_Debug_Que;
    Debug_Status_List[12] = &D_Flash;
    Debug_Status_List[13] = &D_F0;
    Debug_Status_List[14] = &D_F1;
    Debug_Status_List[15] = &D_F2;
    Debug_Status_List[16] = &D_MAP;
    Debug_Status_List[17] = &D_BackPressure;
    Debug_Status_List[18] = &D_MCU_temperature;
    Debug_Status_List[19] = &D_AdditionalAI;
    Debug_Status_List[20] = &D_Input_Analog;
    Debug_Status_List[21] = &D_VSS;
    Debug_Status_List[22] = &D_IGF;
    Debug_Status_List[23] = &D_Input_Frequency;

    // Initialize structures
    for (uint8_t index = 0; index < 19; index++)
    {
        nECU_Debug_ProgramBlockData_Clear(Debug_Status_List[index]);
    }
}
void nECU_Debug_ProgramBlockData_Clear(nECU_ProgramBlockData *inst) // Clear structure 'ProgramBlockData'
{
    memset(inst, 0, sizeof(nECU_ProgramBlockData));
    inst->Status = D_BLOCK_STOP;
    nECU_TickTrack_Init(&(inst->Update_ticks));
}
void nECU_Debug_ProgramBlockData_Update(nECU_ProgramBlockData *inst) // Update tick tracking and check for timeout
{
    nECU_TickTrack_Update(&(inst->Update_ticks));
}