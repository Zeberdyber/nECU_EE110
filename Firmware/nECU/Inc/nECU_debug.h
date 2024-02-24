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

#ifdef __cplusplus
}
#endif

#endif /* _nECU_debug_H__ */