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

/* General code */
void nECU_Start(void) // start executing program (mostly in main loop, some in background with interrupts)
{
    // uint16_t *ptr = NULL; // Initially NULL
    // nECU_Memory_Create(&ptr, 10);
    // HAL_Delay(100);
    bool status = false;
    status |= nECU_Debug_Start(); // MUST BE THE FIRST EXECUTED LINE!!

    if (!nECU_FC_Initialize_Check(D_Main))
    {
        status |= nECU_PC_Start();

        status |= nECU_FLASH_Start(); // initialize FLASH module -> copy from FLASH to RAM
        // nECU_EGT_Start();
        // status |= nECU_test(); // perform system and code tests

        status |= Frame0_Start();
        status |= Frame1_Start();
        status |= Frame2_Start();
        status |= nECU_CAN_Start();

        status |= OnBoard_LED_Start();

        if (!status)
            status |= !nECU_FC_Initialize_Do(D_Main);
    }
    if (!nECU_FC_Working_Check(D_Main) && status == false)
    {
        status |= nECU_InputAnalog_Start(ADC_VREF_ID);

        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_Main);
        }
    }
    if (status)
    {
        nECU_FC_Error_Do(D_Main);
    }
    // vrefADC = nECU_ADC_getPointer(ADC_VREF_ID);
    // nECU_Delay_Set(&max_decay, 1000);
    // nECU_Delay_Start(&max_decay);
    // nECU_TIM_PWM_Start(TIM_PWM_LED1_ID, 0);
    // nECU_TIM_PWM_Start(TIM_PWM_LED2_ID, 0);
    // nECU_FC_Error_Do(D_Main);
}
extern uint16_t Memory_Used;
void nECU_main(void) // main rutine of the program
{
    if (nECU_FC_Error_Check(D_Main))
        while (1)
            HAL_Delay(1);

    if (!nECU_FC_Working_Check(D_Main))
    {
        nECU_FC_Error_Do(D_Main);
        return;
    }

    // // call periodic functions
    // nECU_Knock_UpdatePeriodic();
    // nECU_EGT_Routine();
    // nECU_Menu_Routine();

    // // update all variables for CAN transmission
    // Frame0_PrepareBuffer();
    // Frame1_PrepareBuffer();
    // Frame2_PrepareBuffer();

    // // checks if its time to send packet
    // nECU_CAN_TX_CheckTime();

    // OnBoard_LED_Update();
    // nECU_Debug_Periodic();

    // nECU_InputAnalog_Routine(ADC_VREF_ID);

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
    if (nECU_InputAnalog_Routine(ADC_VREF_ID))
        printf("%d\n\r", (int)nECU_InputAnalog_getValue(ADC_VREF_ID));
    // fflush(stdout);
    // printf("%d\n\r", (int)Memory_Used);

    nECU_FC_Timeout_Check(D_Main);

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
        status |= nECU_FC_Stop_Do(D_Main);

    if (status)
        nECU_FC_Error_Do(D_Main);
}
