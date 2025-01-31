/**
 ******************************************************************************
 * @file    nECU_main.c
 * @brief   This file is the main file of this project. It calls all startup
 *          and loop functions.
 ******************************************************************************
 */

#include "nECU_main.h"

extern nECU_ProgramBlockData D_Main; // diagnostic and flow control data

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    if (D_Main.Status == D_BLOCK_STOP)
    {
        nECU_Debug_Start(); // MUST BE THE FIRST EXECUTED LINE!!

        nECU_PC_Init();

        nECU_FLASH_Start(); // initialize FLASH module -> copy from FLASH to RAM

        nECU_test(); // perform system and code tests

        Frame0_Start();
        Frame1_Start();
        Frame2_Start();
        nECU_CAN_Start();

        OnBoard_LED_Init();

        printf("Main loop -> STARTED!\n");
        D_Main.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
}
void nECU_main(void) // main rutine of the program
{
    if (!(D_Main.Status & D_BLOCK_WORKING))
    {
        D_Main.Status |= D_BLOCK_CODE_ERROR;
        return;
    }

    // call periodic functions
    nECU_Knock_UpdatePeriodic();
    EGT_Periodic();
    nECU_ADC_All_Routine();
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

    nECU_Debug_ProgramBlockData_Update(&D_Main);
    nECU_Debug_ProgramBlockData_Check();
}
void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    ADC_STOP_ALL();
    nECU_Stock_Stop();
    nECU_Button_Stop();
    nECU_CAN_Stop();
    nECU_Knock_Stop();

    printf("Main loop -> STOPPED!\n");
    D_Main.Status -= D_BLOCK_INITIALIZED_WORKING;
}
