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
#define TIM_CLOCK 84000000                  // in Hz from clock config
#define BUTTON_LIGHT_MINIMUM_INFILL 8       // in %
#define BUTTON_LIGHT_MAXIMUM_INFILL 95      // in %
#define BUTTON_LIGHT_RESTING_BRIGHNESS 10   // in %
#define BUTTON_LIGHT_TIME_TO_REST 10000     // in ms
#define BUTTON_LIGHT_ANIMATION_PRESCALER 10 // how many PWM cycles to make until animation update
#define BUTTON_LIGHT_BLINKING_SLOWDOWN 50   // slow down multiplier
#define BUTTON_LIGHT_BREATHING_SPEEDUP 2    // speed up multiplier

#define BUTTON_INPUT_MINIMUM_PULSE_TIME 300 // minimum pulse time to be considered valid [ms]
#define BUTTON_INPUT_HOLD_TIME 1500         // minimal pulse lenght to be consider holding [ms]
#define BUTTON_INPUT_SINGLE_CLICK_TIME 500  // minimal pulse lenght for beeing single click [ms]
#define BUTTON_INPUT_DOUBLE_CLICK_TIME 1400 // maximum time double click must be [ms]

  /* Start/Stop functions */
  void Button_Start(void);
  void Button_Stop(void);

  /* Output functions */
  void ButtonLight_Init(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim); // function to initialize ButtonLight object with corresponding timer
  void ButtonLight_Update(ButtonLight *Light);                                         // periodic animation update function
  void ButtonLight_Set_Blink(ButtonLight *Light, uint8_t Speed, uint16_t Count);       // setup blinking animation
  void ButtonLight_Set_Breathe(ButtonLight *Light, uint8_t Speed, uint16_t Count);     // setup breathing animation
  void ButtonLight_Set_Mode(ButtonLight *Light, ButtonLight_Mode Mode);                // setup mode (animation type)
  void ButtonLight_UpdateAll(void);                                                    // function to launch updates for all buttons
  void ButtonLight_TimeTrack(ButtonLight *Light);                                      // funtion called to update time passed
  void ButtonLight_TimeTrack_All(void);                                                // function called to update time in all buttons
  void ButtonLight_Stop(ButtonLight *Light);                                           // stops button code

  /* Input functions */
  void ButtonInput_Init(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim); // function to initialize ButtonInput object with corresponding timer and GPIO
  void ButtonInput_TimingEvent(TIM_HandleTypeDef *htim);                                // funtion called after input capture interrupt from timer
  void ButtonInput_Identify(TIM_HandleTypeDef *htim);                                   // function to identify to which button is pressed
  void ButtonInput_InterruptRoutine(ButtonInput *button);                               // routine to be called after input capture callback (updates button structure)
  Button_ClickType ButtonInput_GetType(Button_ID id);                                   // get click type if avaliable
  void ButtonInput_Stop(ButtonInput *button);                                           // stop Input Capture for selected button

  /* Animations */
  void ButtonLight_BreathAllOnce(void);                                 // breath all button lights once
  void ButtonLight_BlinkAllOnce(void);                                  // blink all button lights once
  void ButtonLight_BreathOneOnce(Button_ID id);                         // breath selected button once
  void ButtonLight_BlinkOneOnce(Button_ID id);                          // blink selected button once
  void ButtonLight_SetOne(Button_ID id, bool state);                    // set selected button
  void ButtonLight_Breath(Button_ID id, uint8_t Speed, uint16_t Count); // breath one button
  void ButtonLight_Blink(Button_ID id, uint8_t Speed, uint16_t Count);  // blink one button

#ifdef __cplusplus
}
#endif

#endif /* _NECU_BUTTON_H_ */