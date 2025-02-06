/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

static nECU_Debug dbg_data = {0};

static nECU_ProgramBlockData Debug_Status_List[D_ID_MAX] = {0}; // array of all nECU_ProgramBlockData variables

static char const *const D_ID_Strings[D_ID_MAX] = {
    // nECU_adc.c
    [D_ADC1] = "ADC1",
    [D_ADC2] = "ADC2",
    [D_ADC3] = "ADC3",
    // nECU_button.c
    [D_Button_Red] = "Button RED",
    [D_Button_Orange] = "Button ORANGE",
    [D_Button_Green] = "Button Green",
    // nECU_can.c
    [D_CAN_TX] = "CAN TX",
    [D_CAN_RX] = "CAN RX",
    // nECU_data_processing.c
    [D_Data_Processing] = "Data Processing",
    // nECU_debug.c
    [D_Debug] = "Debug Main",
    [D_Debug_Que] = "Debug Que",
    // nECU_EGT.c
    [D_EGT1] = "EGT1",
    [D_EGT2] = "EGT2",
    [D_EGT3] = "EGT3",
    [D_EGT4] = "EGT4",
    // nECU_flash.c
    [D_Flash] = "Flash",
    // nECU_frames.c
    [D_F0] = "CAN Frame 0",
    [D_F1] = "CAN Frame 1",
    [D_F2] = "CAN Frame 2",
    // nECU_Input_Analog.c
    [D_MAP] = "Stock MAP",
    [D_BackPressure] = "BackPressure",
    [D_MCU_temperature] = "nECU temperature",
    [D_AdditionalAI] = "Additional Analog Input",
    // nECU_Input_Frequency.c
    [D_VSS] = "Stock VSS",
    [D_IGF] = "Stock Ignition Fault Detection",
    [D_Input_Frequency] = "Frequency Input",
    // nECU_Knock.c
    [D_Knock] = "Stock Knock sensing",
    // nECU_main.c
    [D_Main] = "Main Loop",
    // nECU_menu.c
    [D_Tacho] = "Tacho Display",
    [D_Menu] = "Button Menu",
    // nECU_OnBoardLED.c
    [D_OnboardLED] = "LED on PCB",
    // nECU_PC.c
    [D_PC] = "PC communication",
    // nECU_Speed.c
    [D_SS1] = "ABS Speed Sensor 1",
    [D_SS2] = "ABS Speed Sensor 2",
    [D_SS3] = "ABS Speed Sensor 3",
    [D_SS4] = "ABS Speed Sensor 4",
    // nECU_stock.c
    [D_GPIO] = "GPIO",
    [D_OX] = "Stock Lambda sensor",
    // nECU_tim.c
    [D_TIM_PWM_BUTTON] = "Button PWM Timer",
    [D_TIM_PWM_OX] = "Stock Lambda Heater PWM Timer",
    [D_TIM_IC_BUTTON] = "Button IC Timer",
    [D_TIM_IC_FREQ] = "Frequency Input Timer",
    [D_TIM_ADC_KNOCK] = "Knock ADC Timer",
    [D_TIM_FRAME] = "Frame Timer",
}; // List of strings of corresponding IDs

/* Debug main functions */
bool nECU_Debug_Start(void) // starts up debugging functions
{
    bool status = false;

    nECU_Debug_ProgramBlock_Init();

    if (!nECU_FlowControl_Initialize_Check(D_Debug))
    {
        status |= nECU_Debug_Init_Struct();
        status |= nECU_Debug_Init_Que();
        status |= nECU_InternalTemp_Start();
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

    /* Device temperature */
    dbg_data.device_temperature.MCU = nECU_InternalTemp_getPointer();

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

    nECU_Debug_IntTemp_Check(&(dbg_data.device_temperature));
    nECU_Debug_EGTTemp_Check(&(dbg_data.egt_temperature));
    nECU_Debug_EGTsensor_error(&(dbg_data.egt_communication));
    nECU_Debug_CAN_Check();
    nECU_Debug_SPI_Check();
    nECU_Debug_ProgramBlockData_Update(D_Debug);
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
    if (!nECU_FlowControl_Working_Check(D_CAN_TX) || !nECU_FlowControl_Working_Check(D_CAN_RX)) // Check if RX or TX is working
    {
        if (nECU_CAN_GetError())
        {
            uint32_t error = HAL_CAN_GetError(&hcan1);
            nECU_Debug_error_mesage temp;
            nECU_Debug_Message_Set(&temp, error, nECU_ERROR_CAN_ID);
        }
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
    printf("New error written to Debug_Que. Value at flag %9.2f, ID: %d\n", inst->value_at_flag, inst->ID);
}

/* Program Block */
static void nECU_Debug_ProgramBlock_Init(void) // Initialize 'ProgramBlock' tracking
{
    // Initialize structures
    for (uint8_t index = 0; index < D_ID_MAX; index++)
    {
        nECU_Debug_ProgramBlockData_Clear(&Debug_Status_List[index]);
    }
    // Configure timeout values
}
static void nECU_Debug_ProgramBlockData_Clear(nECU_ProgramBlockData *inst) // Clear structure 'ProgramBlockData'
{
    memset(inst, 0, sizeof(nECU_ProgramBlockData));
    inst->Status = D_BLOCK_STOP;
    nECU_TickTrack_Init(&(inst->Update_ticks));
    inst->timeout_value = PROGRAMBLOCK_TIMEOUT_DEFAULT * (1000 / (inst->Update_ticks.convFactor));
}
void nECU_Debug_ProgramBlockData_Update(nECU_Module_ID ID) // Update tick tracking and check for timeout
{
    nECU_ProgramBlockData *inst = &Debug_Status_List[ID];
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
    for (uint8_t index = 0; index < D_ID_MAX; index++)
    {
        single_result = nECU_Debug_ProgramBlockData_Check_Single(&Debug_Status_List[index]);
        if (single_result == 2) // save error if major
        {
            nECU_Debug_error_mesage temp;
            float error_message = index;    // copy index to new float
            error_message /= 100;           // move index after 'dot'
            error_message += HAL_GetTick(); // add current tick to the value

            /* Example of how message will work:
                Error detected at index 31, current tick 123456
                error_message == 123456.31;
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
nECU_ProgramBlockData *nECU_Debug_ProgramBlockData_getPointer_Block(nECU_Module_ID ID) // returns pointer to given ID program block
{
    return &(Debug_Status_List[ID]);
}
uint32_t *nECU_Debug_ProgramBlockData_getPointer_Diff(nECU_Module_ID ID) // returns pointer to time difference
{
    return &(Debug_Status_List[ID].Update_ticks.difference);
}

/* Flow control
    ProgramBlock status has 9 states where each bit of byte represents one state:
        D_BLOCK_NULL = 0,        // Block was not initialized
        D_BLOCK_STOP = 1,        // Block stopped
        D_BLOCK_INITIALIZED = 2, // Block structures initialized
        D_BLOCK_WORKING = 4,     // Block working
        D_BLOCK_SPARE_1 = 8,
        D_BLOCK_SPARE_2 = 16,
        D_BLOCK_SPARE_3 = 32,
        D_BLOCK_ERROR_OLD = 64,  // error in memory
        D_BLOCK_ERROR = 128,     // error active

        Usually program should go in such scheme:
            STOP->INITIALIZED->WORKING->STOP->WORKING->STOP... [Initialization is done only once]
        Active statuses are indicated with 'true' at correct bit.
        Some statuses may be actve with others, ex INITIALIZED will be active after initialization and will stay this way

        When error is detected call Error_Do function. It will store state ERROR_OLD. If the same block will call for the second time error will be considered to be major and block will be stopped!

    ProgramBlock has watchdog functionality.
        When timeout_value is reached, block will be stopped!
        Remember to call watchdog function to prevent stop.
        Call frequency can be read from tick difference between watchdog calls.

    If one ProgramBlock is dependent on resources from other:
        At '_Start()' it should call for corresponding '_Start()' function,
        At '_Stop()' it should firstly switch to STOP mode and then attempt '_Stop()' on other ProgramBlocks
            If ProgramBlock A uses data from ProgramBlock B: A_Stop() then B_Stop()

    ProgramBlock cannot be started if it has active ERROR status.
*/
bool nECU_FlowControl_Stop_Check(nECU_Module_ID ID) // Check if block has "initialized" status
{
    if (ID >= D_ID_MAX)
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_STOP);
}
bool nECU_FlowControl_Stop_Do(nECU_Module_ID ID) // Write "initialized" status if possible
{
    if (ID >= D_ID_MAX)
        return false;

    switch (ID) /* Stopping of shared resources */
    {
    case D_ADC1:
        if (nECU_FlowControl_Working_Check(D_AdditionalAI) || nECU_FlowControl_Working_Check(D_BackPressure) || nECU_FlowControl_Working_Check(D_MAP) || nECU_FlowControl_Working_Check(D_MCU_temperature) || nECU_FlowControl_Working_Check(D_OX))
            return false;
        break;
    case D_ADC2:
        if (nECU_FlowControl_Working_Check(D_SS1) || nECU_FlowControl_Working_Check(D_SS2) || nECU_FlowControl_Working_Check(D_SS3) || nECU_FlowControl_Working_Check(D_SS4))
            return false;
        break;

    default:
        break;
    }

    printf("Stopping %s.", D_ID_Strings[ID]);
    if (nECU_FlowControl_Stop_Check(ID) || nECU_FlowControl_Working_Check(ID)) // check if already done
    {
        nECU_FlowControl_Error_Do(ID); // indicate error in code
        return false;
    }
    Debug_Status_List[ID].Status &= ~D_BLOCK_WORKING; // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_STOP;
    return nECU_FlowControl_Initialize_Check(ID);
}

bool nECU_FlowControl_Initialize_Check(nECU_Module_ID ID) // Check if block has "initialized" status
{
    if (ID >= D_ID_MAX)
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_INITIALIZED);
}
bool nECU_FlowControl_Initialize_Do(nECU_Module_ID ID) // Write "initialized" status if possible
{
    if (ID >= D_ID_MAX)
        return false;
    printf("Initializing %s.", D_ID_Strings[ID]);
    if (nECU_FlowControl_Initialize_Check(ID) || !nECU_FlowControl_Stop_Check(ID)) // check if already done
    {
        nECU_FlowControl_Error_Do(ID); // indicate error in code
        return false;
    }

    Debug_Status_List[ID].Status &= ~D_BLOCK_STOP;       // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_INITIALIZED; // Add "INITIALIZED" flag
    return nECU_FlowControl_Initialize_Check(ID);
}

bool nECU_FlowControl_Working_Check(nECU_Module_ID ID) // Check if block has "working" status
{
    if (ID >= D_ID_MAX)
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_WORKING);
}
bool nECU_FlowControl_Working_Do(nECU_Module_ID ID) // Write "working" status if possible
{
    if (ID >= D_ID_MAX)
        return false;
    printf("Starting %s.", D_ID_Strings[ID]);
    if (nECU_FlowControl_Working_Check(ID) || !nECU_FlowControl_Initialize_Check(ID) || !nECU_FlowControl_Error_Check(ID)) // check if already done or it is in error
    {
        nECU_FlowControl_Error_Do(ID); // indicate error in code
        return false;
    }
    Debug_Status_List[ID].Status &= ~D_BLOCK_STOP;   // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_WORKING; // Add "WORKING" flag
    return nECU_FlowControl_Working_Check(ID);
}

bool nECU_FlowControl_Error_Check(nECU_Module_ID ID) // Check if block has "error" status
{
    if (ID >= D_ID_MAX)
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_ERROR);
}
bool nECU_FlowControl_Error_Do(nECU_Module_ID ID) // Write "error" status if possible
{
    if (ID >= D_ID_MAX)
        return false;
    if (nECU_FlowControl_Error_Check(ID) && !nECU_FlowControl_Initialize_Check(ID)) // check if already done
    {
        // Perform action for error on non-initialized block
        return false; // indicate error in code
    }

    Debug_Status_List[ID].Status &= ~D_BLOCK_STOP;   // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_WORKING; // Add "WORKING" flag
    return nECU_FlowControl_Error_Check(ID);
}

bool nECU_FlowControl_DoubleError_Check(nECU_Module_ID ID) // Check if block has "error_old" status
{
    if (ID >= D_ID_MAX)
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_ERROR_OLD);
}
bool nECU_FlowControl_DoubleError_Do(nECU_Module_ID ID) // Write "error_old" status if possible
{
    if (ID >= D_ID_MAX)
        return false;
    if (nECU_FlowControl_Error_Check(ID) && nECU_FlowControl_DoubleError_Check(ID)) // check if already done
    {
        // Perform action for recouring error!!!
        return false; // indicate error in code
    }
    else if (!nECU_FlowControl_Error_Check(ID))
    {
        // Perform action for inproper function call
        return false; // indicate error in code
    }

    Debug_Status_List[ID].Status &= ~D_BLOCK_STOP;     // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_ERROR_OLD; // Add "WORKING" flag
    return nECU_FlowControl_DoubleError_Check(ID);
}