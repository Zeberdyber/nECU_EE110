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
    nECU_Debug_Start(); // MUST BE THE FIRST EXECUTED LINE!!

    nECU_FLASH_Init(); // initialize FLASH module -> copy from FLASH to RAM

    // nECU_systest_run();
    // nECU_codetest_run();
    // nECU_smoothing_tests();

    Frame0_Init();
    Frame1_Init();
    Frame2_Init();
    nECU_CAN_Start();

    OnBoard_LED_Init();

    nECU_IGF_Init();
    nECU_PC_Recieve();
}
void nECU_main(void) // main rutine of the program
{
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

    nECU_IGF_Calc();

    test_uart();

    nECU_Debug_ProgramBlockData_Update(&D_Main);
}
void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    ADC_STOP_ALL();
    nECU_Stock_Stop();
    Button_Stop();
    nECU_CAN_Stop();
    nECU_Knock_DeInit();
}
