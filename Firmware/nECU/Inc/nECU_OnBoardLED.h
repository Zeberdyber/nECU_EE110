/**
 ******************************************************************************
 * @file    nECU_OnBoardLED.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_OnBoardLED.c file
 */
#ifndef _NECU_ONBOARDLED_H_
#define _NECU_ONBOARDLED_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_types.h"
#include "nECU_debug.h"

/* Definitions */
#define ONBOARDLED_BLINK_CONTINOUS UINT8_MAX // value to be set when continous blinking is requested

    /* Function Prototypes */
    /* GPIO */
    void OnBoard_LED_GPIO_Init(OnBoardLED *inst, uint16_t GPIO_Pin, GPIO_TypeDef *GPIOx); // initializes single LED GPIO structure
    void OnBoard_LED_GPIO_Update(GPIO_struct *inst, GPIO_PinState State);                 // update of GPIO output
    /* Animation */
    void OnBoard_LED_Animation_Init(OnBoardLED_Animate *inst, OnBoardLED_Animate_ID priority);      // initializes animation structure
    void OnBoard_LED_Animation_BlinkSetDelay(OnBoardLED_Animate *inst, uint32_t delay);             // sets delay for blinking
    void OnBoard_LED_Animation_BlinkStart(OnBoardLED_Animate *inst, uint32_t delay, uint8_t count); // starts blink animation
    void OnBoard_LED_Animation_BlinkStop(OnBoardLED_Animate *inst);                                 // stops blink animation
    void OnBoard_LED_Animation_BlinkUpdate(OnBoardLED_Animate *inst);                               // updates the blinking animation
    void OnBoard_LED_State_Set(OnBoardLED_Animate *inst, GPIO_PinState State);                      // sets the current state
    void OnBoard_LED_State_Flip(OnBoardLED_Animate *inst);                                          // flips the current state
    /* Animation que */
    void OnBoard_LED_Que_Init(OnBoardLED *inst);                                  // initialize que data struct
    void OnBoard_LED_Que_Add(OnBoardLED *inst, OnBoardLED_Animate *animation);    // adds to the que
    void OnBoard_LED_Que_Remove(OnBoardLED *inst, OnBoardLED_Animate *animation); // removes from the que
    void OnBoard_LED_Que_Check(OnBoardLED *inst);                                 // check if current animation is done, move que
    /* General */
    void OnBoard_LED_Init(void);                      // initialize structures for on board LEDs
    void OnBoard_LED_Update(void);                    // update on board LEDs states
    void OnBoard_LED_Update_Single(OnBoardLED *inst); // update of all internal variables
    /* LED interface functions */
    void OnBoard_LED_L_Add_Animation(OnBoardLED_Animate *animation);
    void OnBoard_LED_L_Remove_Animation(OnBoardLED_Animate *animation);
    void OnBoard_LED_R_Add_Animation(OnBoardLED_Animate *animation);
    void OnBoard_LED_R_Remove_Animation(OnBoardLED_Animate *animation);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_ONBOARDLED_H_ */