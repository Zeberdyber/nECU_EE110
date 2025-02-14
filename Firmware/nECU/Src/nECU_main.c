/**
 ******************************************************************************
 * @file    nECU_main.c
 * @brief   This file is the main file of this project. It calls all startup
 *          and loop functions.
 ******************************************************************************
 */

#include "nECU_main.h"

nECU_ProgramBlockData *block;
uint16_t *vrefADC;

static uint8_t max = 0;
#define bar_len 100
static char bar[bar_len] = {0};
static nECU_Delay max_decay;

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    bool status = false;
    status |= nECU_Debug_Start(); // MUST BE THE FIRST EXECUTED LINE!!

    if (!nECU_FlowControl_Initialize_Check(D_Main))
    {
        status |= nECU_PC_Start();

        status |= nECU_FLASH_Start(); // initialize FLASH module -> copy from FLASH to RAM
        // nECU_EGT_Start();
        status |= nECU_test(); // perform system and code tests

        status |= Frame0_Start();
        status |= Frame1_Start();
        status |= Frame2_Start();
        status |= nECU_CAN_Start();

        status |= OnBoard_LED_Start();

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_Main);
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
    block = nECU_Debug_ProgramBlockData_getPointer_Block(D_Main);
    nECU_InputAnalog_ADC1_Start(ADC1_VREF_ID);
    vrefADC = nECU_ADC1_getPointer(ADC1_VREF_ID);
    nECU_Delay_Set(&max_decay, 1000);
    nECU_Delay_Start(&max_decay);
    nECU_TIM_PWM_Start(TIM_PWM_LED1_ID, 0);
    nECU_TIM_PWM_Start(TIM_PWM_LED2_ID, 0);
}
void nECU_main(void) // main rutine of the program
{
    if (nECU_FlowControl_Error_Check(D_Main))
        while (1)
            HAL_Delay(1);

    if (!nECU_FlowControl_Working_Check(D_Main))
    {
        nECU_FlowControl_Error_Do(D_Main);
        return;
    }

    // call periodic functions
    nECU_Knock_UpdatePeriodic();
    nECU_EGT_Routine();
    nECU_Menu_Routine();

    // update all variables for CAN transmission
    Frame0_PrepareBuffer();
    Frame1_PrepareBuffer();
    Frame2_PrepareBuffer();

    // checks if its time to send packet
    nECU_CAN_TX_CheckTime();

    OnBoard_LED_Update();
    // nECU_Debug_Periodic();

    // nECU_InputAnalog_ADC1_Routine(ADC1_VREF_ID);

    // // nECU_EGT_Routine();

    // nECU_Delay_Update(&max_decay);
    // if (max_decay.done)
    // {
    //     nECU_Delay_Start(&max_decay);
    //     max--;
    // }
    // if (max > block->Update_ticks.difference)
    //     max = (uint8_t)block->Update_ticks.difference;

    // nECU_console_progressBar(bar, bar_len, max);
    // printf("%d\t%s\r", (int)nECU_InputAnalog_ADC1_getValue(ADC1_VREF_ID), bar);
    // fflush(stdout);

    nECU_Debug_ProgramBlockData_Update(D_Main);

    // HAL_Delay(10);

    // test_uart();

    // nECU_Debug_ProgramBlockData_Check();
}
void nECU_Stop(void) // stop all peripherals (no interrupts will generate)
{
    bool status = false;
    status |= nECU_CAN_Stop();
    status |= nECU_Knock_Stop();
    if (!status)
        status |= nECU_FlowControl_Stop_Do(D_Main);

    if (status)
        nECU_FlowControl_Error_Do(D_Main);
}
