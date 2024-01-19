/**
 ******************************************************************************
 * @file    ProcessData.c
 * @brief   This file provides code for preparation and processing of RAW input
 *          data to make it usable for CAN transmission. It consists of
 *          calibration data and corresponding methods.
 ******************************************************************************
 */

#include "ProcessData.h"

uint16_t blank16bit = 0;
uint8_t blank8bit = 0;

uint16_t loopCounter = 0;
bool Initialized = false;
extern bool Knock_UART_Transmission;

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    nECU_FLASH_getAllMemory();
    ADC_START_ALL();
    nECU_Stock_Start();
    Speed_Start();
    EGT_Start();
    Button_Start();
    TachoValue_Init_All();
    Frame0_Init(TachoValue_Get_ShowPointer(1), TachoValue_Get_ShowPointer(2), TachoValue_Get_ShowPointer(3), Button_Menu_getPointer_Antilag(), Button_Menu_getPointer_TractionOFF(), Button_Menu_getPointer_ClearEngineCode(), Button_Menu_getPointer_LunchControlLevel());
    Frame1_Init(TachoValue_Get_OutputPointer(1), TachoValue_Get_OutputPointer(2), TachoValue_Get_OutputPointer(3), Button_Menu_getPointer_TuneSelector());
    Frame2_Init(nECU_BackPressure_GetPointer(), nECU_OX_GetPointer(), nECU_MAP_GetPointer(), nECU_Knock_GetPointer(), nECU_VSS_GetPointer()); // missing knock and OX data filled with blanks
    nECU_CAN_Start();
    Button_Menu_Init();
    OnBoard_LED_Init();
    nECU_UART_RXStartPC();
    nECU_Knock_Init();
    Initialized = true;
}
void nECU_main(void) // main rutine of the program
{
    if (Initialized == true)
    {
        /* High prioryty [time critical] */
        nECU_ADC_All_Routine();
        Button_Menu();
        Frame0_PrepareBuffer();
        Frame1_PrepareBuffer();
        Frame2_PrepareBuffer();
        TachoValue_Update_All();
        ButtonLight_UpdateAll();
        /* Low priority [non-critical] */
        nECU_Stock_Update();
        OnBoard_LED_Update();
        ADC_LP_Update();
        EGT_PeriodicEventLP();
        nECU_Delay_UpdateAll();
    }
    if (Knock_UART_Transmission == true)
    {
        nECU_ADC_All_Routine();
    }
    loopCounter++;
}

void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    ADC_STOP_ALL();
    nECU_Stock_Stop();
    Button_Stop();
    nECU_CAN_Stop();
    nECU_UART_RXStopPC();
    nECU_Knock_DeInit();
    Initialized = false;
}
