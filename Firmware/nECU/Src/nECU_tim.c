/**
 ******************************************************************************
 * @file    nECU_tim.c
 * @brief   This file provides code for user defined timer functions.
 ******************************************************************************
 */

#include "nECU_tim.h"

static nECU_Delay Flash_save_delay;
static nECU_Delay Knock_rotation_delay;
extern nECU_InternalTemp MCU_temperature;

static nECU_tim_Watchdog Button_Out_Watchdog, Ox_Out_Watchdog;

extern VSS_Handle VSS;
extern IGF_Handle IGF;

/* Used in watchdogs */
extern Oxygen_Handle OX;
extern Button Red;

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
      nECU_tim_IC_Callback(&(IGF.tim), &(IGF.ic));
      break;
    case HAL_TIM_ACTIVE_CHANNEL_2:
      nECU_tim_IC_Callback(&(VSS.tim), &(VSS.ic));
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
  nECU_TickTrack_Init(&(inst->timeTrack));
  inst->timePassed = 0;
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
    nECU_TickTrack_Update(&(inst->timeTrack));
    inst->timePassed += inst->timeTrack.difference;

    if (inst->timePassed > inst->timeSet)
    {
      inst->done = true;
      inst->active = false;
    }
  }
}
void nECU_Delay_Stop(nECU_Delay *inst) // stop non-blocking delay
{
  inst->done = false;
  inst->active = false;
}
void nECU_Delay_UpdateAll(void) // update all created non-blocking delays
{
  nECU_Delay_Update(&Flash_save_delay);
  nECU_Delay_Update(&Knock_rotation_delay);
  nECU_Delay_Update(&(MCU_temperature.Update_Delay));
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
  uint32_t delay = (120000 / *rpm) / HAL_GetTickFreq(); // 120000 = 120 (Hz to rpm) * 1000 (ms to s)
  nECU_Delay_Set(&Knock_rotation_delay, &delay);
  nECU_Delay_Start(&Knock_rotation_delay);
}

/* Delay for internal temperature update */
bool *nECU_InternalTemp_Delay_DoneFlag(void) // return flag if internal temperature updates is due
{
  return nECU_Delay_DoneFlag(&(MCU_temperature.Update_Delay));
}
void nECU_InternalTemp_Delay_Start(void) // start non-blocking delay for internal temperature updates
{
  uint32_t delay = INTERNAL_TEMP_UPDATE_DELAY / HAL_GetTickFreq();
  nECU_Delay_Set(&(MCU_temperature.Update_Delay), &delay);
  nECU_Delay_Start(&(MCU_temperature.Update_Delay));
}

/* general nECU timer functions */
nECU_TIM_State nECU_tim_PWM_start(nECU_Timer *tim) // function to start PWM on selected timer
{
  nECU_TIM_State result = TIM_OK;

  for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++)
  {
    if (HAL_TIM_PWM_Start_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // start pwm, check result
    {
      result = TIM_ERROR; // indicate if not successful
      break;
    }
  }
  return result;
}
nECU_TIM_State nECU_tim_PWM_stop(nECU_Timer *tim) // function to stop PWM on selected timer
{
  nECU_TIM_State result = TIM_OK;

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

  return result;
}
nECU_TIM_State nECU_tim_IC_start(nECU_Timer *tim) // function to start IC on selected timer
{
  nECU_TIM_State result = TIM_OK;

  for (uint8_t Channel = 0; Channel < tim->Channel_Count; Channel++)
  {
    if (HAL_TIM_IC_Start_IT(tim->htim, tim->Channel_List[Channel]) != HAL_OK) // start IC, check result
    {
      result = TIM_ERROR; // indicate if not successful
      break;
    }
  }

  return result;
}
nECU_TIM_State nECU_tim_IC_stop(nECU_Timer *tim) // function to stop IC on selected timer
{
  nECU_TIM_State result = TIM_OK;

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
  return result;
}
nECU_TIM_State nECU_tim_base_start(nECU_Timer *tim) // function to start base of selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_Base_Start_IT(tim->htim))
  {
    result = TIM_ERROR; // indicate if not successful
  }
  return result;
}
nECU_TIM_State nECU_tim_base_stop(nECU_Timer *tim) // function to stop base of selected timer
{
  nECU_TIM_State result = TIM_OK;
  if (HAL_TIM_Base_Stop_IT(tim->htim))
  {
    result = TIM_ERROR; // indicate if not successful
  }
  return result;
}

void nECU_tim_Init_struct(nECU_Timer *tim) // initialize structure and precalculate variables
{
  tim->refClock = TIM_CLOCK / (tim->htim->Init.Prescaler + 1);                                 // add 1 for prescaler nature
  tim->period = 1 / tim->refClock;                                                             // calculate period of a single increment in timer
  tim->Channel_Count = 0;                                                                      // clear count
  for (uint8_t channel = 0; channel < sizeof(tim->Channel_List) / sizeof(uint32_t); channel++) // zero out every position of the list
  {
    tim->Channel_List[channel] = 0;
  }
}

void nECU_tim_IC_Callback(nECU_Timer *tim, nECU_InputCapture *ic) // callback function to calculate basic parameters
{
  uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(tim->htim, tim->htim->Channel);

  /* Calculate difference */
  uint16_t Difference = 0; // in miliseconds
  if (ic->previous_CCR > CurrentCCR)
  {
    Difference = ((tim->htim->Init.Period + 1) - ic->previous_CCR) + CurrentCCR;
  }
  else
  {
    Difference = CurrentCCR - ic->previous_CCR;
  }
  ic->previous_CCR = CurrentCCR;
  ic->frequency = tim->refClock / Difference;
}

/* Watchdog for timers detection */
void nECU_tim_Watchdog_Init(void) // initialize structure
{
  // Button out timer
  nECU_tim_Watchdog_Init_struct(&Button_Out_Watchdog);
  Button_Out_Watchdog.tim = &(Red.light.Timer);

  // Ox out timer
  nECU_tim_Watchdog_Init_struct(&Ox_Out_Watchdog);
  Ox_Out_Watchdog.tim = &(OX.Heater);
}
void nECU_tim_Watchdog_Init_struct(nECU_tim_Watchdog *watchdog) // set default values to variables
{
  watchdog->error = false;
  watchdog->warning = false;
  nECU_TickTrack_Init(&(watchdog->timeTrack));
  watchdog->counter_max = watchdog->tim->period * (watchdog->tim->htim->Init.Period + 1) * WATCHDOG_PERIOD_MULTIPLIER;
}

void nECU_tim_Watchdog_Periodic(void) // watchdog function for active timers
{
  /* Get time difference of function calls */

  /* PWM Outputs */
  nECU_tim_Watchdog_CheckStates(&Button_Out_Watchdog);   // state of peripheral and outputs
  nECU_tim_Watchdog_updateCounter(&Button_Out_Watchdog); // update watchdog
  nECU_tim_Watchdog_CheckCounter(&Button_Out_Watchdog);  // check if counter in bounds

  nECU_tim_Watchdog_CheckStates(&Ox_Out_Watchdog);   // state of peripheral and outputs
  nECU_tim_Watchdog_updateCounter(&Ox_Out_Watchdog); // update watchdog
  nECU_tim_Watchdog_CheckCounter(&Ox_Out_Watchdog);  // check if counter in bounds
}
void nECU_tim_Watchdog_updateCounter(nECU_tim_Watchdog *watchdog) // update counter value based on systick
{
  nECU_TickTrack_Update(&(watchdog->timeTrack)); // update tracker (get tick difference)

  watchdog->counter_ms += watchdog->timeTrack.difference * HAL_GetTickFreq(); // add diference as time
}
void nECU_tim_Watchdog_Callback(TIM_HandleTypeDef *htim) // function to be called on timer interrupt
{
  nECU_tim_Watchdog *inst;

  /* find which watchdog is responsible */
  if (htim == Button_Out_Watchdog.tim->htim)
  {
    inst = &Button_Out_Watchdog;
  }
  else if (htim == Ox_Out_Watchdog.tim->htim)
  {
    inst = &Ox_Out_Watchdog;
  }

  /* perform watchdog update */
  inst->counter_ms = 0;
  nECU_TickTrack_Init(&(inst->timeTrack));
}

bool nECU_tim_Watchdog_CheckStates(nECU_tim_Watchdog *watchdog) // check state of peripheral
{
  if (watchdog->tim->htim->State == HAL_TIM_STATE_RESET) // check if peripheral in use
  {
    return false; // peripheral not initialized
  }
  if (watchdog->tim->htim->State == HAL_TIM_STATE_ERROR) // chcek if peripheral error
  {
    watchdog->error = true;
    return true; // peripheral error
  }
  if (nECU_tim_Watchdog_CheckChannels(watchdog->tim))
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
