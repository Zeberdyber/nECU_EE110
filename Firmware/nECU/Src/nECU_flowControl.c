/**
 ******************************************************************************
 * @file    nECU_flowControl.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_flowControl.h"

nECU_ProgramBlockData Debug_Status_List[D_ID_MAX] = {0}; // array of all nECU_ProgramBlockData variables

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
    [D_Frame_Speed_ID] = "CAN Frame 0",
    [D_Frame_EGT_ID] = "CAN Frame 1",
    [D_Frame_Stock_ID] = "CAN Frame 2",
    // nECU_Input_Analog.c
    [D_ANALOG_MAP] = "Stock MAP",
    [D_ANALOG_BackPressure] = "BackPressure",
    [D_ANALOG_OX] = "OX analog sensor",
    [D_ANALOG_AI_1] = "Additional Analog Input 1",
    [D_ANALOG_AI_2] = "Additional Analog Input 2",
    [D_ANALOG_AI_3] = "Additional Analog Input 3",
    [D_ANALOG_MCUTemp] = "nECU temperature",
    [D_ANALOG_VREF] = "Reference voltage sensing",
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
    [D_ANALOG_SS1] = "ABS Speed Sensor 1",
    [D_ANALOG_SS2] = "ABS Speed Sensor 2",
    [D_ANALOG_SS3] = "ABS Speed Sensor 3",
    [D_ANALOG_SS4] = "ABS Speed Sensor 4",
    // nECU_stock.c
    [D_DigiInput_CRANKING] = "Digital Input Cranking",
    [D_DigiInput_FAN_ON] = "Digital Input Radiator Fan",
    [D_DigiInput_LIGHTS_ON] = "Digital Input Headlights",
    [D_DigiInput_VSS] = "Digital Input VSS (Edge Sensing)",
    [D_DigiInput_IGF] = "Digital Input IGF (Edge Sensing)",
    [D_OX] = "Stock Lambda sensor",
    // nECU_tim.c
    [D_TIM_PWM_BUTTON] = "Button PWM Timer",
    [D_TIM_PWM_OX] = "Stock Lambda Heater PWM Timer",
    [D_TIM_IC_BUTTON] = "Button IC Timer",
    [D_TIM_IC_FREQ] = "Frequency Input Timer",
    [D_TIM_ADC_KNOCK] = "Knock ADC Timer",
    [D_TIM_FRAME] = "Frame Timer",
    [D_TIM_PWM_LED1] = "OnBoard LED1 Timer",
    [D_TIM_PWM_LED2] = "OnBoard LED2 Timer",
}; // List of strings of corresponding IDs

/* Program Block */
void nECU_Debug_ProgramBlock_Init(void) // Initialize 'ProgramBlock' tracking
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
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_STOP);
}
bool nECU_FlowControl_Stop_Do(nECU_Module_ID ID) // Write "initialized" status if possible
{
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;

    switch (ID) /* Stopping of shared resources */
    {
    case D_ADC1:
        if (nECU_FlowControl_Working_Check(D_ANALOG_MAP) || nECU_FlowControl_Working_Check(D_ANALOG_BackPressure) || nECU_FlowControl_Working_Check(D_ANALOG_OX) || nECU_FlowControl_Working_Check(D_ANALOG_AI_1) || nECU_FlowControl_Working_Check(D_ANALOG_AI_2) || nECU_FlowControl_Working_Check(D_ANALOG_AI_3) || nECU_FlowControl_Working_Check(D_ANALOG_MCUTemp) || nECU_FlowControl_Working_Check(D_ANALOG_VREF))
            return false;
        break;
    case D_ADC2:
        if (nECU_FlowControl_Working_Check(D_ANALOG_SS1) || nECU_FlowControl_Working_Check(D_ANALOG_SS2) || nECU_FlowControl_Working_Check(D_ANALOG_SS3) || nECU_FlowControl_Working_Check(D_ANALOG_SS4))
            return false;
        break;
    case D_Flash:
        if (nECU_FlowControl_Working_Check(D_ANALOG_SS1) || nECU_FlowControl_Working_Check(D_ANALOG_SS2) || nECU_FlowControl_Working_Check(D_ANALOG_SS3) || nECU_FlowControl_Working_Check(D_ANALOG_SS4) || nECU_FlowControl_Working_Check(D_Debug_Que) || nECU_FlowControl_Working_Check(D_Menu))
            return false;
        break;

    default:
        break;
    }

    if (nECU_FlowControl_Working_Check(D_PC))
        printf("Stopping %s.\n\r", D_ID_Strings[ID]);
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
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_INITIALIZED);
}
bool nECU_FlowControl_Initialize_Do(nECU_Module_ID ID) // Write "initialized" status if possible
{
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;

    if (nECU_FlowControl_Working_Check(D_PC))
        printf("\rInitializing %s.\n\r", D_ID_Strings[ID]);
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
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_WORKING);
}
bool nECU_FlowControl_Working_Do(nECU_Module_ID ID) // Write "working" status if possible
{
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;

    if (nECU_FlowControl_Working_Check(D_PC))
        printf("Starting %s.\n\r", D_ID_Strings[ID]);
    if (nECU_FlowControl_Working_Check(ID) || !nECU_FlowControl_Initialize_Check(ID) || nECU_FlowControl_Error_Check(ID)) // check if already done or it is in error
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
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_ERROR);
}
bool nECU_FlowControl_Error_Do(nECU_Module_ID ID) // Write "error" status if possible
{
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    if (nECU_FlowControl_Error_Check(ID) && !nECU_FlowControl_Initialize_Check(ID)) // check if already done
    {
        if (nECU_FlowControl_Working_Check(D_PC))
            printf("Error detected at %s - was not initialized.\n\r", D_ID_Strings[ID]);

        // Perform action for error on non-initialized block
        return false; // indicate error in code
    }
    if (nECU_FlowControl_Working_Check(D_PC))
        printf("Error detected at %s - general error.\n\r", D_ID_Strings[ID]);

    // Debug_Status_List[ID].Status &= ~D_BLOCK_STOP;   // Clear "STOP" flag
    Debug_Status_List[ID].Status |= D_BLOCK_ERROR; // Add "ERROR" flag
    return nECU_FlowControl_Error_Check(ID);
}

bool nECU_FlowControl_DoubleError_Check(nECU_Module_ID ID) // Check if block has "error_old" status
{
    if (ID >= D_ID_MAX) // Break if invalid ID
        return false;
    return (bool)(Debug_Status_List[ID].Status & D_BLOCK_ERROR_OLD);
}
bool nECU_FlowControl_DoubleError_Do(nECU_Module_ID ID) // Write "error_old" status if possible
{
    if (ID >= D_ID_MAX) // Break if invalid ID
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