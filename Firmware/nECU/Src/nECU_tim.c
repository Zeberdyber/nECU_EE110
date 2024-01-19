/**
 ******************************************************************************
 * @file    nECU_tim.c
 * @brief   This file provides code for user defined timer functions.
 ******************************************************************************
 */

#include "nECU_tim.h"

nECU_Delay Flash_save_delay;
nECU_Delay Knock_rotation_delay;

uint8_t nECU_Get_FrameTimer(void) // get current value of frame timer
{
  /* timer 11 is set to work with 0,1ms count up time, and have 8bit period*/
  if (HAL_TIM_STATE_READY == HAL_TIM_Base_GetState(&FRAME_TIMER)) // if not working (not busy)
  {
    HAL_TIM_Base_Start(&FRAME_TIMER);
  }
  return (FRAME_TIMER.Instance->CNT);
}

/* Callback functions */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &CAN_LOW_PRIORITY_TIMER || htim == &CAN_HIGH_PRIORITY_TIMER) // if one of timing timers
  {
    nECU_CAN_TimerEvent(htim);
  }
  else if (htim == &KNOCK_REGRES_TIMER)
  {
    nECU_Knock_UpdatePeriodic();
    nECU_VSS_DetectZero(htim);
  }
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &BUTTON_OUTPUT_TIMER) // routine for TIM1
  {
    ButtonLight_TimingEvent();
  }
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &BUTTON_INPUT_TIMER)
  {
    ButtonInput_TimingEvent(htim);
  }
  else if (htim == &FREQ_INPUT_TIMER)
  {
    switch (htim->Channel)
    {
    case HAL_TIM_ACTIVE_CHANNEL_1:
      nECU_RPM_Calc();
      break;
    case HAL_TIM_ACTIVE_CHANNEL_2:
      nECU_VSS_Update();
      break;

    default:
      break;
    }
  }
}

bool *nECU_Delay_DoneFlag(nECU_Delay *inst) // return done flag pointer of non-blocking delay
{
  return &(inst->done);
}
void nECU_Delay_Start(nECU_Delay *inst) // start non-blocking delay
{
  inst->timeStart = HAL_GetTick();
  inst->done = false;
  inst->active = true;
}
void nECU_Delay_Set(nECU_Delay *inst, uint32_t *delay) // preset the non-blocking delay
{
  inst->timeSet = *delay;
}
void nECU_Delay_Update(nECU_Delay *inst) // update current state of non-blocking delay
{
  // it is not a time critical function, just lunch it inside main loop
  // nECU_Delay functions are based on tick setting, refer to HAL_Delay() function
  if (inst->done == false && inst->active == true) // do only if needed
  {
    if (inst->timeStart > HAL_GetTick()) // check if value overload
    {
      if ((inst->timeStart + inst->timeSet) > (HAL_GetTick() + 0xFFFFFFFF))
      {
        inst->done = true;
        inst->active = false;
      }
    }
    else
    {
      if ((inst->timeStart + inst->timeSet) < HAL_GetTick())
      {
        inst->done = true;
        inst->active = false;
      }
    }
  }
}
void nECU_Delay_UpdateAll(void) // update all created non-blocking delays
{
  nECU_Delay_Update(&Flash_save_delay);
  nECU_Delay_Update(&Knock_rotation_delay);
}

/* Flash save user setting delay */
bool *nECU_Save_Delay_DoneFlag(void) // return flag if save is due
{
  return nECU_Delay_DoneFlag(&Flash_save_delay);
}
void nECU_Save_Delay_Start(void) // start non-blocking delay for save
{
  uint32_t delay = FLASH_SAVE_DELAY_TIME / HAL_GetTickFreq();
  nECU_Delay_Set(&Flash_save_delay, &delay);
  nECU_Delay_Start(&Flash_save_delay);
}

/* Delay knock update to next cycle */
bool *nECU_Knock_Delay_DoneFlag(void) // return flag if knock is due
{
  return nECU_Delay_DoneFlag(&Knock_rotation_delay);
}
void nECU_Knock_Delay_Start(float *rpm) // start non-blocking delay for knock
{
  uint32_t delay = (120000 / *rpm) / HAL_GetTickFreq();
  nECU_Delay_Set(&Knock_rotation_delay, &delay);
  nECU_Delay_Start(&Knock_rotation_delay);
}