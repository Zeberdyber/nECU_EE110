/**
 ******************************************************************************
 * @file    nECU_tim.c
 * @brief   This file provides code for user defined timer functions.
 ******************************************************************************
 */

#include "nECU_tim.h"

static nECU_Delay Flash_save_delay;
static nECU_Delay Knock_rotation_delay;

static nECU_tim_Watchdog Button_Out_Watchdog, Ox_Out_Watchdog;

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
  nECU_tim_Watchdog_Callback(htim);
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
      nECU_IGF_Calc();
      break;
    case HAL_TIM_ACTIVE_CHANNEL_2:
      nECU_VSS_Update();
      break;

    default:
      break;
    }
  }
}

/* Non-blocking delay */
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
      if ((inst->timeStart + inst->timeSet) > (HAL_GetTick() + UINT32_MAX))
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

/* general nECU timer functions */
nECU_TIM_State nECU_tim_PWM_start(nECU_Timer *tim) // function to start PWM on selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_PWM_GetState(tim->htim) == HAL_TIM_STATE_RESET) // check if pwm is not working
  {
    for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++)
    {
      if (HAL_TIM_PWM_Start_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // start pwm, check result
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
    }
  }
  return result;
}
nECU_TIM_State nECU_tim_PWM_stop(nECU_Timer *tim) // function to stop PWM on selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_PWM_GetState(tim->htim) != HAL_TIM_STATE_RESET) // check if pwm is working
  {
    for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++) // do for every channel
    {
      if (HAL_TIM_PWM_Stop_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // stop pwm, check result
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
      if (HAL_TIM_GetChannelState(tim->htim, tim->Channel_List[Channel]) != HAL_TIM_CHANNEL_STATE_RESET) // confirm status of channel
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
    }
  }
  return result;
}
nECU_TIM_State nECU_tim_IC_start(nECU_Timer *tim) // function to start IC on selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_IC_GetState(tim->htim) == HAL_TIM_STATE_RESET) // check if IC is not working
  {
    for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++)
    {
      if (HAL_TIM_IC_Start_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // start IC, check result
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
    }
  }
  return result;
}
nECU_TIM_State nECU_tim_IC_stop(nECU_Timer *tim) // function to stop IC on selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_IC_GetState(tim->htim) != HAL_TIM_STATE_RESET) // check if IC is working
  {
    for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++) // do for every channel
    {
      if (HAL_TIM_IC_Stop_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // stop IC, check result
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
      if (HAL_TIM_GetChannelState(tim->htim, tim->Channel_List[Channel]) != HAL_TIM_CHANNEL_STATE_RESET) // confirm status of channel
      {
        result = TIM_ERROR; // indicate if not successful
        break;
      }
    }
  }
  return result;
}

void nECU_tim_Init_struct(nECU_Timer *tim) // initialize structure and precalculate variables
{
  tim->refClock = TIM_CLOCK / ((tim->htim->Init.Prescaler + 1) * (tim->htim->Init.ClockDivision >> 2)); // add 1 for prescaler nature, move by 2 to decode clock division
  tim->period = 1 / tim->refClock;                                                                      // calculate period of a single increment in timer
  tim->Channel_Count = 0;                                                                               // clear count
  for (uint8_t channel = 0; channel < sizeof(tim->Channel_List) / sizeof(uint32_t); channel++)          // zero out every position of the list
  {
    tim->Channel_List[channel] = 0;
  }
}

/* Watchdog for timers detection */
void nECU_tim_Watchdog_Init(void) // initialize structure
{
  // Button out timer
  nECU_tim_Watchdog_Init_struct(&Button_Out_Watchdog);
  Button_Out_Watchdog.tim.htim = &OX_HEATER_TIMER;
  Button_Out_Watchdog.tim.Channel_Count = 3;
  Button_Out_Watchdog.tim.Channel_List[0] = TIM_CHANNEL_1;
  Button_Out_Watchdog.tim.Channel_List[1] = TIM_CHANNEL_2;
  Button_Out_Watchdog.tim.Channel_List[2] = TIM_CHANNEL_3;

  // Ox out timer
  nECU_tim_Watchdog_Init_struct(&Ox_Out_Watchdog);
  Ox_Out_Watchdog.tim.htim = &BUTTON_OUTPUT_TIMER;
  Ox_Out_Watchdog.tim.Channel_Count = 1;
  Ox_Out_Watchdog.tim.Channel_List[0] = TIM_CHANNEL_1;
}
void nECU_tim_Watchdog_Init_struct(nECU_tim_Watchdog *watchdog) // set default values to variables
{
  watchdog->error = false;
  watchdog->warning = false;
  watchdog->previousTick = 0;
  nECU_tim_Init_struct(&watchdog->tim);
  watchdog->counter_max = watchdog->tim.period * (watchdog->tim.htim->Init.Period + 1) * WATCHDOG_PERIOD_MULTIPLIER;
}

void nECU_tim_Watchdog_Periodic(void) // watchdog function for active timers
{
  /* Get time difference of function calls */

  /* PWM Outputs */
  nECU_tim_Watchdog_CheckStates(&Button_Out_Watchdog);
  nECU_tim_Watchdog_updateCounter(&Button_Out_Watchdog);
  nECU_tim_Watchdog_CheckCounter(&Button_Out_Watchdog);

  nECU_tim_Watchdog_CheckStates(&Ox_Out_Watchdog);
  nECU_tim_Watchdog_updateCounter(&Ox_Out_Watchdog);
  nECU_tim_Watchdog_CheckCounter(&Ox_Out_Watchdog);
}
void nECU_tim_Watchdog_updateCounter(nECU_tim_Watchdog *watchdog) // update counter value based on systick
{
  uint32_t CurrentTick = HAL_GetTick() * HAL_GetTickFreq(); // get time in ms

  if (CurrentTick > watchdog->previousTick) // calculate difference (include tick variable roll over)
  {
    watchdog->counter_ms += CurrentTick - watchdog->previousTick;
  }
  else
  {
    watchdog->counter_ms += CurrentTick + UINT32_MAX - watchdog->previousTick;
  }

  watchdog->previousTick = CurrentTick; // save time for next loop
}
void nECU_tim_Watchdog_Callback(TIM_HandleTypeDef *htim) // function to be called on timer interrupt
{
  if (htim == Button_Out_Watchdog.tim.htim)
  {
    Button_Out_Watchdog.counter_ms = 0;
  }
  else if (htim == Ox_Out_Watchdog.tim.htim)
  {
    Ox_Out_Watchdog.counter_ms = 0;
  }
}

bool nECU_tim_Watchdog_CheckStates(nECU_tim_Watchdog *watchdog) // check state of peripheral
{
  if (watchdog->tim.htim->State == HAL_TIM_STATE_RESET) // check if peripheral in use
  {
    return false; // peripheral not initialized
  }
  if (watchdog->tim.htim->State == HAL_TIM_STATE_ERROR) // chcek if peripheral error
  {
    watchdog->error = true;
    return true; // peripheral error
  }
  if (nECU_tim_Watchdog_CheckChannels(&watchdog->tim))
  {
    watchdog->error = true;
    return true; // channel error
  }
  return false; // timer is OK
}
bool nECU_tim_Watchdog_CheckCounter(nECU_tim_Watchdog *watchdog) // check counter, determine timer error
{
  if (watchdog->counter_max < watchdog->counter_ms)
  {
    watchdog->error = true;
    return true;
  }
  return false;
}
bool nECU_tim_Watchdog_CheckChannels(nECU_Timer *tim) // check channels of timer
{
  for (uint8_t i = 0; i < tim->Channel_Count; i++)
  {
    if (HAL_TIM_GetChannelState(tim->htim, tim->Channel_List[i]))
    {
      return true;
    }
  }
  return false;
}
