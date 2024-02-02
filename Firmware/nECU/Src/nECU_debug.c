/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

OnBoardLED LED_L, LED_R;
bool LED_Initialized = false; // flag to triger initialization

/* On board LEDs */
void OnBoard_LED_Init(void) // initialize structures for on board LEDs
{
    /* Left LED */
    LED_L.GPIO_Pin = LED1_Pin;
    LED_L.GPIOx = LED1_GPIO_Port;
    LED_L.LastTick = HAL_GetTick();

    /* Right LED */
    LED_R.GPIO_Pin = LED2_Pin;
    LED_R.GPIOx = LED2_GPIO_Port;
    LED_R.LastTick = HAL_GetTick();

    LED_Initialized = true;
}
void OnBoard_LED_UpdateSingle(OnBoardLED *inst) // function to perform logic behind blinking times and update to GPIO
{
    if (inst->blinking == true)
    {
        if ((HAL_GetTick() - inst->LastTick) > ((float)(ONBOARD_LED_MS_PER_BLINK / 2) * HAL_GetTickFreq()))
        {
            inst->LastTick = HAL_GetTick();
            inst->BlinkState = !inst->BlinkState;
        }
        inst->State = inst->BlinkState;
    }
    else
    {
        inst->LastTick = HAL_GetTick();
    }
    HAL_GPIO_WritePin(inst->GPIOx, inst->GPIO_Pin, inst->State);
}
void OnBoard_LED_Update(void) // update on board LEDs states
{
    if (LED_Initialized == false)
    {
        OnBoard_LED_Init();
    }

    LED_L.State = nECU_CAN_GetError();
    LED_L.blinking = nECU_CAN_GetState();
    OnBoard_LED_UpdateSingle(&LED_L);

    LED_R.State = nECU_SPI_getError(&SPI_PERIPHERAL_EGT);
    LED_R.blinking = nECU_SPI_getBusy(&SPI_PERIPHERAL_EGT);
    OnBoard_LED_UpdateSingle(&LED_R);
}

void nECU_Fault_Missfire(void) // routine after missfire was detected
{
}