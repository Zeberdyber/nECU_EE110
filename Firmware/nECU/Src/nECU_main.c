/**
 ******************************************************************************
 * @file    nECU_main.c
 * @brief   This file provides code for preparation and processing of RAW input
 *          data to make it usable for CAN transmission. It consists of
 *          calibration data and corresponding methods.
 ******************************************************************************
 */

#include "nECU_main.h"

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    // nECU_systest_run();
    // nECU_codetest_run();

    nECU_FLASH_getAllMemory();
    Button_Start();

    Frame0_Init();
    Frame1_Init();
    Frame2_Init();
    nECU_CAN_Start();

    // OnBoard_LED_Init();
    // nECU_Debug_Init_Que();
}
void nECU_main(void) // main rutine of the program
{
    // call periodic functions
    nECU_Knock_UpdatePeriodic();
    EGT_Periodic();
    nECU_ADC_All_Routine();
    Button_Menu();
    nECU_Delay_UpdateAll();
    ButtonLight_UpdateAll();

    // update all variables for CAN transmission
    Frame0_PrepareBuffer();
    Frame1_PrepareBuffer();
    Frame2_PrepareBuffer();

    // checks if its time to send packet
    nECU_CAN_CheckTime();

    // OnBoard_LED_Update();
    // add debug funcion call
}
void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    ADC_STOP_ALL();
    nECU_Stock_Stop();
    Button_Stop();
    nECU_CAN_Stop();
    nECU_Knock_DeInit();
}
