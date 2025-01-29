/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

static nECU_Debug dbg_data = {0};

// Program status control
nECU_ProgramBlockData *Debug_Status_List[34] = {0};                                             // array of pointers to all nECU_ProgramBlockData
nECU_ProgramBlockData D_ADC1, D_ADC2, D_ADC3;                                                   // nECU_adc.c
nECU_ProgramBlockData D_Button_Red, D_Button_Orange, D_Button_Green;                            // nECU_button.c
nECU_ProgramBlockData D_CAN_TX, D_CAN_RX;                                                       // nECU_can.c
nECU_ProgramBlockData D_Data_Processing;                                                        // nECU_data_processing.c
nECU_ProgramBlockData D_Debug, D_Debug_Que;                                                     // nECU_debug.c
nECU_ProgramBlockData D_Flash;                                                                  // nECU_flash.c
nECU_ProgramBlockData D_F0, D_F1, D_F2;                                                         // nECU_frames.c
nECU_ProgramBlockData D_MAP, D_BackPressure, D_MCU_temperature, D_AdditionalAI, D_Input_Analog; // nECU_Input_Analog.c
nECU_ProgramBlockData D_VSS, D_IGF, D_Input_Frequency;                                          // nECU_Input_Frequency.c
nECU_ProgramBlockData D_Knock;                                                                  // nECU_Knock.c
nECU_ProgramBlockData D_Main;                                                                   // nECU_main.c
nECU_ProgramBlockData D_Tacho, D_Menu;                                                          // nECU_menu.c
nECU_ProgramBlockData D_OnboardLED;                                                             // nECU_OnBoardLED.c
nECU_ProgramBlockData D_PC;                                                                     // nECU_PC.c
nECU_ProgramBlockData D_SS1, D_SS2, D_SS3, D_SS4;                                               // nECU_Speed.c
nECU_ProgramBlockData D_GPIO, D_OX;                                                             // nECU_stock.c

/* Debug main functions */
bool nECU_Debug_Start(void) // starts up debugging functions
{
    bool status = false;

    nECU_Debug_ProgramBlock_Init();

    if (D_Debug.Status == D_BLOCK_STOP)
    {
        status |= nECU_Debug_Init_Struct();
        status |= nECU_Debug_Init_Que();
        status |= nECU_InternalTemp_Init();
        D_Debug.Status |= D_BLOCK_INITIALIZED;
    }

    if (D_Debug.Status & D_BLOCK_INITIALIZED)
    {
        D_Debug.Status |= D_BLOCK_WORKING;
    }

    return status;
}
static bool nECU_Debug_Init_Struct(void) // set values to variables in structure
{
    bool status = false;

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

    return status;
}
void nECU_Debug_Periodic(void) // checks states of variables
{
    if (!(D_Debug.Status & D_BLOCK_INITIALIZED_WORKING))
    {
        return;
    }

    nECU_Debug_IntTemp_Check(&(dbg_data.device_temperature));
    nECU_Debug_EGTTemp_Check(&(dbg_data.egt_temperature));
    nECU_Debug_EGTsensor_error(&(dbg_data.egt_communication));
    nECU_Debug_CAN_Check();
    nECU_Debug_SPI_Check();
    nECU_Debug_ProgramBlockData_Update(&D_Debug);
}

/* Check states routines */
static void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst) // check for errors of device temperature
{
    if (*nECU_InternalTemp_StartupDelay_DoneFlag() == true) // do after startup delay is done
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->MCU)) // check main IC
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, *(inst->MCU), nECU_ERROR_DEVICE_TEMP_MCU_ID);
        }
    }
    for (uint8_t i = 0; i < 4; i++) // check all EGT devices
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, *(inst->EGT_IC[i]), nECU_ERROR_DEVICE_TEMP_EGT1_ID + i);
        }
    }
}
static bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature) // checks if passed temperature is in defined bounds
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
static void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst) // check if TCs did not exceed fault value
{
    for (uint8_t i = 0; i < 4; i++)
    {
        if (nECU_Debug_EGTTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, *(inst->EGT_IC[i]), nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
static bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature) // checks if passed temperature is in defined bound
{
    if (*temperature > TC_TEMPERATURE_MAX)
    {
        return true;
    }
    return false;
}
static void nECU_Debug_EGTsensor_error(nECU_Debug_EGT_Comm *inst) // check EGT ICs for error flags
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
            nECU_Debug_Message_Set(&temp, (float)*inst->EGT_IC[i], nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
static void nECU_Debug_CAN_Check(void) // checks if CAN have any error pending
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
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_CAN_ID);
        }
    }
}
static void nECU_Debug_SPI_Check(void) // checks if SPI have any error pending
{
    if (HAL_SPI_GetState(&SPI_PERIPHERAL_EGT) == HAL_SPI_STATE_ERROR)
    {
        uint32_t error = HAL_SPI_GetError(&SPI_PERIPHERAL_EGT);
        if (error > HAL_SPI_ERROR_NONE)
        {
            SPI_PERIPHERAL_EGT.ErrorCode = HAL_SPI_ERROR_NONE; // resets error
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

    if (D_Debug_Que.Status == D_BLOCK_STOP)
    {
        dbg_data.error_que.counter.preset = sizeof(dbg_data.error_que.messages) / sizeof(nECU_Debug_error_mesage); // calculate length of que
        dbg_data.error_que.counter.value = 0;
        dbg_data.error_que.message_count = 0;
        for (uint16_t que_index = 0; que_index < dbg_data.error_que.counter.preset; que_index++)
        {
            nECU_Debug_Message_Init(&(dbg_data.error_que.messages[que_index])); // clear each
        }
        status |= nECU_readDebugQue(&(dbg_data.error_que)); // on this run it will only set pointer for further reading/writing
        D_Debug_Que.Status |= D_BLOCK_INITIALIZED_WORKING;
    }

    return status;
}
static void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message) // add message to debug que
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
}

/* Program Block */
static void nECU_Debug_ProgramBlock_Init(void) // Initialize 'ProgramBlock' tracking
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
    Debug_Status_List[10] = &D_Debug_Que;
    Debug_Status_List[11] = &D_Flash;
    Debug_Status_List[12] = &D_F0;
    Debug_Status_List[13] = &D_F1;
    Debug_Status_List[14] = &D_F2;
    Debug_Status_List[15] = &D_MAP;
    Debug_Status_List[16] = &D_BackPressure;
    Debug_Status_List[17] = &D_MCU_temperature;
    Debug_Status_List[18] = &D_AdditionalAI;
    Debug_Status_List[19] = &D_Input_Analog;
    Debug_Status_List[20] = &D_VSS;
    Debug_Status_List[21] = &D_IGF;
    Debug_Status_List[22] = &D_Input_Frequency;
    Debug_Status_List[23] = &D_Knock;
    Debug_Status_List[24] = &D_Main;
    Debug_Status_List[25] = &D_Tacho;
    Debug_Status_List[26] = &D_Menu;
    Debug_Status_List[27] = &D_OnboardLED;
    Debug_Status_List[28] = &D_SS1;
    Debug_Status_List[29] = &D_SS2;
    Debug_Status_List[30] = &D_SS3;
    Debug_Status_List[31] = &D_SS4;
    Debug_Status_List[32] = &D_GPIO;
    Debug_Status_List[33] = &D_OX;

    // Initialize structures
    for (uint8_t index = 0; index < (sizeof(Debug_Status_List) / sizeof(Debug_Status_List[0])); index++)
    {
        nECU_Debug_ProgramBlockData_Clear(Debug_Status_List[index]);
    }

    // Configure timeout values
}
static void nECU_Debug_ProgramBlockData_Clear(nECU_ProgramBlockData *inst) // Clear structure 'ProgramBlockData'
{
    memset(inst, 0, sizeof(nECU_ProgramBlockData));
    inst->Status = D_BLOCK_STOP;
    nECU_TickTrack_Init(&(inst->Update_ticks));
    inst->timeout_value = PROGRAMBLOCK_TIMEOUT_DEFAULT;
}
void nECU_Debug_ProgramBlockData_Update(nECU_ProgramBlockData *inst) // Update tick tracking and check for timeout
{
    nECU_TickTrack_Update(&(inst->Update_ticks));
    if (inst->Update_ticks.difference > inst->timeout_value)
    {
        inst->Status |= D_BLOCK_ERROR;
    }
}
void nECU_Debug_ProgramBlockData_Check(void) // Perform error check for all blocks
{
    uint8_t single_result;
    // Perform for each one
    for (uint8_t index = 0; index < (sizeof(Debug_Status_List) / sizeof(Debug_Status_List[0])); index++)
    {
        single_result = nECU_Debug_ProgramBlockData_Check_Single(Debug_Status_List[index]);
        if (single_result == 2) // save error if major
        {
            nECU_Debug_error_mesage temp;
            float error_message = index;    // copy index to new float
            error_message /= 1000;          // move index after 'dot'
            error_message += HAL_GetTick(); // add current tick to the value

            /* Example of how message will work:
                Error detected at index 31, current tick 123456
                error_message == 123456.031;
            */
            nECU_Debug_Message_Set(&temp, error_message, nECU_ERROR_PROGRAMBLOCK);
        }
    }
}
static uint8_t nECU_Debug_ProgramBlockData_Check_Single(nECU_ProgramBlockData *inst) // returns true if issue in given instance
{
    /* Return value scheme:
        0 - OK
        1 - NOK -> low priority
        2 - NOK -> persistent error
    */
    uint8_t result = 0;

    if (inst->Status & D_BLOCK_ERROR)
    {
        result = 1;
        if (inst->Status & D_BLOCK_ERROR_OLD) // if error was already in memor (major error)
        {
            result = 2;
        }
        inst->Status |= D_BLOCK_ERROR_OLD; // store error to memory
        inst->Status -= D_BLOCK_ERROR;
    }
    if (inst->Status & D_BLOCK_CODE_ERROR) // major error as programming was bad :O
    {
        result = 2;
    }

    return result;
}