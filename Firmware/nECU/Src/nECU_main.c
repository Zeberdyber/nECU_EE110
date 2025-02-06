/**
 ******************************************************************************
 * @file    nECU_main.c
 * @brief   This file is the main file of this project. It calls all startup
 *          and loop functions.
 ******************************************************************************
 */

#include "nECU_main.h"

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    bool status = false;
    status |= nECU_Debug_Start(); // MUST BE THE FIRST EXECUTED LINE!!

    if (!nECU_FlowControl_Initialize_Check(D_Main))
    {
        status |= nECU_PC_Start();

        status |= nECU_FLASH_Start(); // initialize FLASH module -> copy from FLASH to RAM

        status |= nECU_test(); // perform system and code tests

        status |= Frame0_Start();
        status |= Frame1_Start();
        status |= Frame2_Start();
        status |= nECU_CAN_Start();

        status |= OnBoard_LED_Start();

        if (!status)
            status |= nECU_FlowControl_Initialize_Do(D_Main);
    }
    if (!nECU_FlowControl_Working_Check(D_Main) && status == false)
    {
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_Main);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Main);
    }
}
void nECU_main(void) // main rutine of the program
{
    if (!nECU_FlowControl_Working_Check(D_Main))
    {
        nECU_FlowControl_Error_Do(D_Main);
        return;
    }

    // call periodic functions
    nECU_Knock_UpdatePeriodic();
    nECU_EGT_Routine();
    Button_Menu();
    nECU_Delay_UpdateAll();

    // update all variables for CAN transmission
    Frame0_PrepareBuffer();
    Frame1_PrepareBuffer();
    Frame2_PrepareBuffer();

    // checks if its time to send packet
    nECU_CAN_CheckTime();

    OnBoard_LED_Update();
    nECU_Debug_Periodic();

    test_uart();

    nECU_Debug_ProgramBlockData_Update(D_Main);
    nECU_Debug_ProgramBlockData_Check();
}
void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    bool status = false;
    status |= nECU_Button_Stop();
    status |= nECU_CAN_Stop();
    status |= nECU_Knock_Stop();
    if (!status)
        status |= nECU_FlowControl_Stop_Do(D_Main);

    if (status)
        nECU_FlowControl_Error_Do(D_Main);
}
