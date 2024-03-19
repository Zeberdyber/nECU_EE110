/**
 ******************************************************************************
 * @file    nECU_debug.c
 * @brief   This file provides code for debugging code using onboard buttons,
 *          and LEDs.
 ******************************************************************************
 */

#include "nECU_debug.h"

OnBoardLED LED_L, LED_R;
static nECU_Debug dbg_data;
static nECU_LoopCounter main_loop;

static bool LED_Initialized = false,
            mainLoop_Initialized = false,
            debug_que_initialized = false;

/* On board LEDs */
void OnBoard_LED_Init(void) // initialize structures for on board LEDs
{
    if (LED_Initialized == false)
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
}
void OnBoard_LED_UpdateSingle(OnBoardLED *inst) // function to perform logic behind blinking times and update to GPIO
{
    if (LED_Initialized == false)
    {
        return;
    }
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
        return;
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
        return;
    }

    HAL_GPIO_TogglePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin);
}
void nECU_LED_SetState(OnBoardLED *inst, GPIO_PinState state) // set state to selected LED
{
    if (LED_Initialized == false)
    {
        return;
    }
    inst->LEDPin.State = state;
    HAL_GPIO_WritePin(inst->LEDPin.GPIOx, inst->LEDPin.GPIO_Pin, inst->LEDPin.State);
}

/* Used to track how many times main loop is done between CAN frames */
void nECU_mainLoop_Init(void)
{
    nECU_LoopCounter_Init(&main_loop);
}
void nECU_mainLoop_Update(void)
{
    nECU_LoopCounter_Update(&main_loop);
}
void nECU_mainLoop_Reset(void)
{
    nECU_LoopCounter_Clear(&main_loop);
}
uint16_t *nECU_mainLoop_getValue(void) // returns pointer to current value
{
    return (uint16_t *)main_loop.counter;
}

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
    inst->time += inst->tracker.difference * inst->tracker.convFactor; // add difference in ms
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
    inst->convFactor = HAL_GetTickFreq();
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
void nECU_Debug_Message_Set(nECU_Debug_error_mesage *inst, float value, nECU_Error_ID ID) // sets error values
{
    inst->error_flag = true;
    inst->value_at_flag = value;
    inst->ID = ID;
    // HERE ADD QUE PUSH
    nECU_Debug_Que_Write(inst);
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

    /* EGT temperature (thermocuple temperature) */
    dbg_data.egt_temperature.EGT_IC[0] = EGT_GetTemperaturePointer(EGT_CYL1);
    dbg_data.egt_temperature.EGT_IC[1] = EGT_GetTemperaturePointer(EGT_CYL2);
    dbg_data.egt_temperature.EGT_IC[2] = EGT_GetTemperaturePointer(EGT_CYL3);
    dbg_data.egt_temperature.EGT_IC[3] = EGT_GetTemperaturePointer(EGT_CYL4);
    nECU_Debug_Message_Init(&(dbg_data.egt_temperature.over_temperature));

    /* EGT communication */
    dbg_data.egt_communication.EGT_IC[0] = EGT_GetErrorState(EGT_CYL1);
    dbg_data.egt_communication.EGT_IC[1] = EGT_GetErrorState(EGT_CYL2);
    dbg_data.egt_communication.EGT_IC[2] = EGT_GetErrorState(EGT_CYL3);
    dbg_data.egt_communication.EGT_IC[3] = EGT_GetErrorState(EGT_CYL4);

    // IMPLEMENT ERROR DETECTION OF SPI COMMUNICATION FOR EACH SENSOR

    nECU_Debug_Message_Init(&(dbg_data.egt_communication.TC_invalid));
}
void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst) // check for errors of device temperature
{
    if (nECU_Debug_IntTemp_CheckSingle(inst->MCU)) // check main IC
    {
        nECU_Debug_Message_Set(&(inst->over_temperature), *(inst->MCU), nECU_ERROR_DEVICE_TEMP_MCU_ID);
    }
    for (uint8_t i = 0; i < 4; i++) // check all EGT devices
    {
        if (nECU_Debug_IntTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_Message_Set(&(inst->over_temperature), *(inst->EGT_IC[i]), nECU_ERROR_DEVICE_TEMP_EGT1_ID + i);
        }
    }
}
bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature) // checks if passed temperature is in defined bounds
{
    /* true => out of bounds, false => no errors */
    if ((*temperature / INTERNAL_TEMP_MULTIPLIER) > DEVICE_TEMPERATURE_MAX)
    {
        return true;
    }
    else if ((*temperature / INTERNAL_TEMP_MULTIPLIER) < DEVICE_TEMPERATURE_MIN)
    {
        return true;
    }
    return false;
}
void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst) // check if TCs did not exceed fault value
{
    for (uint8_t i = 0; i < 4; i++)
    {
        if (nECU_Debug_EGTTemp_CheckSingle(inst->EGT_IC[i]))
        {
            nECU_Debug_Message_Set(&(inst->over_temperature), *(inst->EGT_IC[i]), nECU_ERROR_EGT_TC_EGT1_ID + i);
        }
    }
}
bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature) // checks if passed temperature is in defined bound
{
    if (*temperature > TC_TEMPERATURE_MAX)
    {
        return true;
    }
    return false;
}
void nECU_Debug_EGTsensor_error(EGT_Sensor_ID ID) // check EGT ICs for error flags
{
    for (uint8_t i = 0; i < 4; i++)
    {
        // if (inst->EGT_IC[i]) // if error detected set message
        // {
        //     nECU_Debug_Message_Set(&(inst->TC_invalid), (float)*inst->EGT_IC[i], nECU_ERROR_EGT_TC_EGT1_ID + i);
        // }
    }
}
void nECU_Debug_EGTSPIcomm_error(EGT_Sensor_ID ID) // to be called when SPI error occurs
{
    nECU_Debug_error_mesage temporary;
    nECU_Debug_Message_Init(&temporary);
    nECU_Debug_Message_Set(&temporary, (float)HAL_GetTick(), nECU_ERROR_EGT_SPI_EGT1_ID + ID);
}
void nECU_Debug_FLASH_error(nECU_Flash_Error_ID ID, bool write_read) // indicate error from flash functions
{
    // write_read ==  true => writing, false => reading
    nECU_Debug_error_mesage temporary;
    nECU_Debug_Message_Init(&temporary);
    nECU_Error_ID id;
    switch (ID)
    {
    case nECU_FLASH_ERROR_SPEED:
        id = nECU_ERROR_FLASH_SPEED_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_USER:
        id = nECU_ERROR_FLASH_USER_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_DBGQUE:
        id = nECU_ERROR_FLASH_DEBUG_QUE_SAVE_ID + write_read;
        break;
    case nECU_FLASH_ERROR_ERASE:
        id = nECU_ERROR_FLASH_ERASE_ID;
        break;

    default:
        break;
    }
    nECU_Debug_Message_Set(&temporary, (float)HAL_GetTick(), id);
}

void nECU_Debug_Init_Que(void) // initializes que
{
    dbg_data.error_que.counter.preset = sizeof(dbg_data.error_que.messages) / sizeof(nECU_Debug_error_mesage); // calculate length of que
    dbg_data.error_que.counter.value = 0;
    dbg_data.error_que.message_count = 0;
    for (uint16_t que_index = 0; que_index < dbg_data.error_que.counter.preset; que_index++)
    {
        nECU_Debug_Message_Init(&(dbg_data.error_que.messages[que_index])); // clear each
    }
    debug_que_initialized = true;
}
void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message) // add message to debug que
{
    if (debug_que_initialized == false)
    {
        return;
    }

    dbg_data.error_que.counter.value++;
    dbg_data.error_que.message_count++;
    if (dbg_data.error_que.counter.value == dbg_data.error_que.counter.preset) // check if reached maximum value
    {
        dbg_data.error_que.counter.value = 0;
    }
    if (dbg_data.error_que.message_count >= dbg_data.error_que.counter.preset) // cap the maximum number of messages
    {
        dbg_data.error_que.message_count = dbg_data.error_que.counter.preset;
    }

    memcpy(&(dbg_data.error_que.messages[dbg_data.error_que.counter.value]), message, sizeof(nECU_Debug_error_mesage)); // copy to que
}
void nECU_Debug_Que_Read(nECU_Debug_error_mesage *message) // read newest message from debug que
{
    if (debug_que_initialized == false)
    {
        return;
    }

    if (dbg_data.error_que.message_count == 0) // break if no messages in que
    {
        return;
    }

    memcpy(message, &(dbg_data.error_que.messages[dbg_data.error_que.counter.value]), sizeof(nECU_Debug_error_mesage)); // read last message

    nECU_Debug_Message_Init(&(dbg_data.error_que.messages[dbg_data.error_que.counter.value])); // clear que position

    dbg_data.error_que.counter.value--;
    dbg_data.error_que.message_count--;
}