/**
 ******************************************************************************
 * @file    nECU_button.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_button.c file
 */

#ifndef _NECU_BUTTON_H_
#define _NECU_BUTTON_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "stm32f4xx_hal.h"
#include "nECU_tim.h"
#include "tim.h"

/* Definitions */
#define TIM_CLOCK 84000000                                           // in Hz from clock config
#define BUTTON_LIGHT_MINIMUM_INFILL 8                                // in %
#define BUTTON_LIGHT_MAXIMUM_INFILL 95                               // in %
#define BUTTON_LIGHT_RESTING_BRIGHNESS (BUTTON_LIGHT_MINIMUM_INFILL) // in %
#define BUTTON_LIGHT_TIME_TO_REST 5000                               // in ms
#define BUTTON_LIGHT_BLINKING_SLOWDOWN 100                           // slow down multiplier
#define BUTTON_LIGHT_BREATHING_SLOWDOWN 5                            // slow down multiplier

#define BUTTON_INPUT_MINIMUM_PULSE_TIME 30 // minimum pulse time to be considered valid [ms]
#define BUTTON_INPUT_HOLD_TIME 750         // minimal pulse lenght to be consider holding [ms]
#define BUTTON_INPUT_DOUBLE_CLICK_TIME 700 // maximum time double click must be [ms]

  /* Start/Stop functions */
  bool Button_Start(void);
  void Button_Stop(void);

  /* Output functions */
  static bool ButtonLight_Init(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim); // function to initialize ButtonLight object with corresponding timer
  static void ButtonLight_Update(ButtonLight *Light);                                         // periodic animation update function
  static void ButtonLight_TimeTrack(ButtonLight *Light);                                      // funtion called to update time passed
  static void ButtonLight_Stop(ButtonLight *Light);                                           // stops button code
  void ButtonLight_UpdateAll(void);                                                           // function to launch updates for all buttons

  /* Input functions */
  static bool ButtonInput_Init(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim); // function to initialize ButtonInput object with corresponding timer and GPIO
  static void ButtonInput_Stop(ButtonInput *button);                                           // stop Input Capture for selected button
  void ButtonInput_Identify(TIM_HandleTypeDef *htim);                                          // function to identify to which button is pressed
  static void ButtonInput_InterruptRoutine(ButtonInput *button);                               // routine to be called after input capture callback (updates button structure)
  Button_ClickType ButtonInput_GetType(Button_ID id);                                          // get click type if avaliable

  /* Animations */
  static bool ButtonLight_Identify(Button_ID id, ButtonLight **light);  // find corresponding light structrue, return if ok
  void ButtonLight_SetOne(Button_ID id, bool state);                    // set selected button
  void ButtonLight_Breath(Button_ID id, uint8_t Speed, uint16_t Count); // breath one button
  void ButtonLight_Blink(Button_ID id, uint8_t Speed, uint16_t Count);  // blink one button

#ifdef __cplusplus
}
#endif

#endif /* _NECU_BUTTON_H_ */