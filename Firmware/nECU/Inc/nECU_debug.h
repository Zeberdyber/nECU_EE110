/**
 ******************************************************************************
 * @file    nECU_debug.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_debug.c file
 */
#ifndef nECU_debug_H_
#define nECU_debug_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "nECU_debug.h"
#include "nECU_types.h"
#include "nECU_can.h"
#include "nECU_spi.h"

/* Definitions */
#define ONBOARD_LED_MS_PER_BLINK 1000 // number of miliseconds for full blink

#define DEVICE_TEMPERATURE_MAX 55  // in deg C
#define DEVICE_TEMPERATURE_MIN -20 // in deg C
#define TC_TEMPERATURE_MAX 1000    // in deg C

    /* On board LED */
    void OnBoard_LED_Init(void);                                   // initialize structures for on board LEDs
    void OnBoard_LED_UpdateSingle(OnBoardLED *inst);               // function to perform logic behind blinking times and update to GPIO
    void OnBoard_LED_Update(void);                                 // update on board LEDs states
    void nECU_LED_FlipState(OnBoardLED *inst);                     // simple function for debugging code
    void nECU_LED_SetState(OnBoardLED *inst, GPIO_PinState state); // set state to selected LED

    /* Fault detection */
    void nECU_Fault_Missfire(void); // routine after missfire was detected

    void nECU_LoopCounter_Init(nECU_LoopCounter *inst);   // Initialize structure
    void nECU_LoopCounter_Update(nECU_LoopCounter *inst); // Increment counter, get total time
    void nECU_LoopCounter_Clear(nECU_LoopCounter *inst);  // clear value of the counter

    void nECU_TickTrack_Init(nECU_TickTrack *inst);   // initialize structure
    void nECU_TickTrack_Update(nECU_TickTrack *inst); // callback to get difference

    void nECU_Debug_Message_Init(nECU_Debug_error_mesage *inst);                               // zeros value inside of structure
    void nECU_Debug_Message_Set(nECU_Debug_error_mesage *inst, float value, nECU_Error_ID ID); // sets error values

    void nECU_Debug_Init_Struct(void);                                    // set values to variables in structure
    void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst);              // check for errors of device temperature
    bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature);            // checks if passed temperature is in defined bounds
    void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst);             // check if TCs did not exceed fault value
    bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature);           // checks if passed temperature is in defined bound
    void nECU_Debug_EGTcomm_Check(nECU_Debug_EGT_Comm *inst);             // check EGT ICs for error flags
    void nECU_Debug_FLASH_error(nECU_Flash_Error_ID ID, bool write_read); // indicate error from flash functions

    void nECU_Debug_Init_Que(void);                              // initializes que
    void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message); // add message to debug que
    void nECU_Debug_Que_Read(nECU_Debug_error_mesage *message);  // read newest message from debug que

#ifdef __cplusplus
}
#endif

#endif /* _nECU_debug_H__ */