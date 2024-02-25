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
static nECU_Debug dbg_data;

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
    if (LED_Initialized == false)
    {
        OnBoard_LED_Init();
    }
    inst->LEDPin.State = state;
    HAL_GPIO_WritePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin, inst->LEDPin.State);
}

void nECU_Fault_Missfire(void) // routine after missfire was detected
{
    nECU_LED_FlipState(&LED_L);
}

/* Used to track how many times main loop is done between CAN frames */
void nECU_LoopCounter_Init(nECU_LoopCounter *inst) // Initialize structure
{
    inst->counter = 0;
    nECU_TickTrack_Init(&(inst->tracker));
    inst->time = 0;
}
void nECU_LoopCounter_Update(nECU_LoopCounter *inst) // Increment counter, get total time
{
    inst->counter++;
    nECU_TickTrack_Update(&(inst->tracker));
    inst->time += inst->tracker.difference * HAL_GetTickFreq(); // add difference in ms
}
void nECU_LoopCounter_Clear(nECU_LoopCounter *inst) // clear value of the counter
{
    inst->counter = 0;
    inst->time = 0;
}

/* Used for simple time tracking */
void nECU_TickTrack_Init(nECU_TickTrack *inst) // initialize structure
{
    inst->previousTick = HAL_GetTick();
    inst->difference = 0;
}
void nECU_TickTrack_Update(nECU_TickTrack *inst) // callback to get difference
{
    uint64_t tickNow = HAL_GetTick();
    if (tickNow < inst->previousTick) // check if data roll over
    {
        inst->difference = (tickNow + UINT32_MAX) - inst->previousTick;
    }
    else
    {
        inst->difference = tickNow - inst->previousTick;
    }
    inst->previousTick = tickNow;
}

void nECU_Debug_Message_Init(nECU_Debug_error_mesage *inst) // zeros value inside of structure
{
    inst->error_flag = false;
    inst->ID = 0;
    inst->value_at_flag = 0.0;
}
void nECU_Debug_Message_Set(nECU_Debug_error_mesage *inst, float value, uint8_t ID) // sets error values
{
    inst->error_flag = true;
    inst->value_at_flag = value;
    inst->ID = ID;
}
void nECU_Debug_Init_Struct(void) // set values to variables in structure
{
    /* Device temperature */
    dbg_data.device_temperature.MCU = nECU_InternalTemp_getTemperature();
    dbg_data.device_temperature.EGT_IC[0] = EGT_GetTemperatureInternalPointer(EGT_CYL1);
    dbg_data.device_temperature.EGT_IC[1] = EGT_GetTemperatureInternalPointer(EGT_CYL2);
    dbg_data.device_temperature.EGT_IC[2] = EGT_GetTemperatureInternalPointer(EGT_CYL3);
    dbg_data.device_temperature.EGT_IC[3] = EGT_GetTemperatureInternalPointer(EGT_CYL4);
    nECU_Debug_Message_Init(&(dbg_data.device_temperature.over_temperature));
}
void nECU_Debug_IntTemp_Check(void) // check for errors of device temperature
{
    if (nECU_Debug_IntTemp_CheckSingle(*(dbg_data.device_temperature.MCU))) // check main IC
    {
        nECU_Debug_Message_Set(&(dbg_data.device_temperature.over_temperature), *(dbg_data.device_temperature.MCU), nECU_ERROR_DEVICE_TEMP_MCU_ID);
        return;
    }
    for (uint8_t i = 0; i < 4; i++) // check all EGT devices
    {
        if (nECU_Debug_IntTemp_CheckSingle(&(dbg_data.device_temperature.EGT_IC[i])))
        {
            nECU_Debug_Message_Set(&(dbg_data.device_temperature.over_temperature), *(dbg_data.device_temperature.EGT_IC[i]), nECU_ERROR_DEVICE_TEMP_EGT1_ID + i);
            return;
        }
    }
}
bool nECU_Debug_IntTemp_CheckSingle(int16_t temperature) // checks if passed temperature is in defined bounds
{
    /* true => out of bounds, false => no errors */
    if ((temperature / INTERNAL_TEMP_MULTIPLIER) > DEVICE_TEMPERATURE_MAX)
    {
        return true;
    }
    else if ((temperature / INTERNAL_TEMP_MULTIPLIER) < DEVICE_TEMPERATURE_MIN)
    {
        return true;
    }
    return false;
}