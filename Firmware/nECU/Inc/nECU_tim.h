/**
 ******************************************************************************
 * @file    nECU_tim.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_tim.c file
 */
#ifndef _NECU_TIM_H_
#define _NECU_TIM_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "tim.h"
#include "nECU_can.h"
#include "nECU_button.h"
#include "nECU_Knock.h"
#include "nECU_stock.h"

  /* Definitions */

#define WATCHDOG_PERIOD_MULTIPLIER 2 // after how many missed callbacks an error will be called

  /* Function Prototypes */
  uint8_t nECU_Get_FrameTimer(void); // get current value of frame timer

  static nECU_TIM_ID nECU_TIM_Identify(TIM_HandleTypeDef *htim);               // returns ID of given input
  TIM_HandleTypeDef *nECU_TIM_getPointer(nECU_TIM_ID ID);                      // returns pointer to timer
  nECU_InputCapture *nECU_TIM_IC_getPointer(nECU_TIM_ID ID, uint32_t Channel); // pointer to IC stucture

  /* Callback functions */
  void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
  void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
  void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

  /* Used for simple time tracking */
  bool nECU_TickTrack_Init(nECU_TickTrack *inst);   // initialize structure
  bool nECU_TickTrack_Update(nECU_TickTrack *inst); // callback to get difference

  /* Non-blocking delay */
  bool *nECU_Delay_DoneFlag(nECU_Delay *inst);           // return done flag pointer of non-blocking delay
  bool nECU_Delay_Start(nECU_Delay *inst);               // start non-blocking delay
  bool nECU_Delay_Set(nECU_Delay *inst, uint32_t delay); // preset the non-blocking delay
  bool nECU_Delay_Update(nECU_Delay *inst);              // update current state of non-blocking delay
  bool nECU_Delay_Stop(nECU_Delay *inst);                // stop non-blocking delay

  /* general nECU timer functions */
  bool nECU_TIM_Init(nECU_TIM_ID ID);                                                  // initialize structure and precalculate variables
  bool nECU_TIM_PWM_Start(nECU_TIM_ID ID, uint32_t Channel);                           // function to start PWM on selected timer
  bool nECU_TIM_PWM_Stop(nECU_TIM_ID ID, uint32_t Channel);                            // function to stop PWM on selected timer
  bool nECU_TIM_PWM_Fill(nECU_TIM_ID ID, uint32_t Channel, float Fill);                // function to change PWM duty
  bool nECU_TIM_IC_Start(nECU_TIM_ID ID, uint32_t Channel, nECU_DigiInput_ID Digi_ID); // function to start IC on selected timer
  bool nECU_TIM_IC_Stop(nECU_TIM_ID ID, uint32_t Channel);                             // function to stop IC on selected timer
  bool nECU_TIM_Base_Start(nECU_TIM_ID ID);                                            // function to start base of selected timer
  bool nECU_TIM_Base_Stop(nECU_TIM_ID ID);                                             // function to stop base of selected timer

  static bool nECU_TIM_IC_Callback(nECU_TIM_ID ID); // callback function to calculate basic parameters

  // /* Watchdog for timers detection */
  // bool nECU_tim_Watchdog_Init(void);                                       // initialize structure
  // bool nECU_tim_Watchdog_Init_struct(nECU_tim_Watchdog *watchdog);         // set default values to variables
  // void nECU_tim_Watchdog_Periodic(void);                                   // watchdog function for active timers
  // bool nECU_tim_Watchdog_updateCounter(nECU_tim_Watchdog *watchdog);       // update counter value based on systick
  // bool nECU_tim_Watchdog_Callback(TIM_HandleTypeDef *htim);                // function to be called on timer interrupt
  // static bool nECU_tim_Watchdog_CheckStates(nECU_tim_Watchdog *watchdog);  // check state of peripheral
  // static bool nECU_tim_Watchdog_CheckCounter(nECU_tim_Watchdog *watchdog); // check counter, determine timer error
  // static bool nECU_tim_Watchdog_CheckChannels(nECU_Timer *tim);            // check channels of timer

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TIM_H_ */