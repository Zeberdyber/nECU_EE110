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
#include "tim.h"
#include "nECU_can.h"
#include "nECU_button.h"
#include "nECU_Knock.h"
#include "nECU_stock.h"

/* Definitions */
#define BUTTON_OUTPUT_TIMER htim1
#define OX_HEATER_TIMER htim2
#define BUTTON_INPUT_TIMER htim3
#define FREQ_INPUT_TIMER htim4
#define CAN_LOW_PRIORITY_TIMER htim6
#define CAN_HIGH_PRIORITY_TIMER htim7
#define KNOCK_ADC_SAMPLING_TIMER htim8
#define FRAME_TIMER htim10
#define KNOCK_REGRES_TIMER htim11

#define FLASH_SAVE_DELAY_TIME 5000 // time to wait for before save happens in ms

  /* typedef */
  typedef struct
  {
    uint32_t timeSet;
    uint32_t timeStart;
    bool done;
    bool active;
  } nECU_Delay;

  typedef struct
  {
    TIM_HandleTypeDef *htim;
    float refClock;
    float period; // in ms
  } Timer;

  /* Function Prototypes */
  uint8_t nECU_Get_FrameTimer(void); // get current value of frame timer

  /* Callback functions */
  void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
  void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
  void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

  /* Non-blocking delay */
  bool *nECU_Delay_DoneFlag(nECU_Delay *inst);            // return done flag pointer of non-blocking delay
  void nECU_Delay_Start(nECU_Delay *inst);                // start non-blocking delay
  void nECU_Delay_Set(nECU_Delay *inst, uint32_t *delay); // preset the non-blocking delay
  void nECU_Delay_Update(nECU_Delay *inst);               // update current state of non-blocking delay
  void nECU_Delay_UpdateAll(void);                        // update all created non-blocking delays

  /* Flash save user setting delay */
  bool *nECU_Save_Delay_DoneFlag(void); // return flag if save is due
  void nECU_Save_Delay_Start(void);     // start non-blocking delay for save

  /* Delay knock update to next cycle */
  bool *nECU_Knock_Delay_DoneFlag(void);   // return flag if knock is due
  void nECU_Knock_Delay_Start(float *rpm); // start non-blocking delay for knock

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TIM_H_ */