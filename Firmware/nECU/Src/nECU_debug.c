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
    uint32_t delay = (ONBOARD_LED_MS_PER_BLINK / 2);
    /* Left LED */
    LED_L.LEDPin.GPIO_Pin = LED1_Pin;
    LED_L.LEDPin.GPIOx = LED1_GPIO_Port;
    nECU_Delay_Set(&LED_L.delay, &delay);
    LED_L.blinkPrev = false;

    /* Right LED */
    LED_R.LEDPin.GPIO_Pin = LED2_Pin;
    LED_R.LEDPin.GPIOx = LED2_GPIO_Port;
    nECU_Delay_Set(&LED_R.delay, &delay);
    LED_R.blinkPrev = false;

    LED_Initialized = true;
}
void OnBoard_LED_UpdateSingle(OnBoardLED *inst) // function to perform logic behind blinking times and update to GPIO
{
    nECU_Delay_Update(&inst->delay);
    if (inst->blinking == true)
    {
        if (inst->blinkPrev == false) // for first startup
        {
            inst->blinkPrev = true;
            nECU_Delay_Start(&inst->delay);
        }
        else if (inst->delay.done)
        {

            inst->LEDPin.State = !inst->LEDPin.State;
            nECU_Delay_Start(&inst->delay);
        }
    }
    else if (inst->blinkPrev == true) // stop blink
    {
        inst->blinkPrev = false;
        nECU_Delay_Stop(&inst->delay);
    }
    HAL_GPIO_WritePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin, inst->LEDPin.State);
}
void OnBoard_LED_Update(void) // update on board LEDs states
{
    if (LED_Initialized == false)
    {
        OnBoard_LED_Init();
    }

    // LED_L.LEDPin.State = nECU_CAN_GetError();
    // LED_L.blinking = nECU_CAN_GetState();
    LED_L.blinking = true;
    OnBoard_LED_UpdateSingle(&LED_L);

    LED_R.LEDPin.State = nECU_SPI_getError(&SPI_PERIPHERAL_EGT);
    LED_R.blinking = nECU_SPI_getBusy(&SPI_PERIPHERAL_EGT);
    OnBoard_LED_UpdateSingle(&LED_R);
}
void nECU_LED_FlipState(OnBoardLED *inst) // simple function for debugging code
{
    if (LED_Initialized == false)
    {
        OnBoard_LED_Init();
    }

    HAL_GPIO_TogglePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin);
}
void nECU_LED_SetState(OnBoardLED *inst, GPIO_PinState state) // set state to selected LED
{
    inst->LEDPin.State = state;
    HAL_GPIO_WritePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin, inst->LEDPin.State);
}

void nECU_Fault_Missfire(void) // routine after missfire was detected
{
}
