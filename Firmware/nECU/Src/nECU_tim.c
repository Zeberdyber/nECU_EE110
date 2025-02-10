/**
 ******************************************************************************
 * @file    nECU_tim.c
 * @brief   This file provides code for user defined timer functions.
 ******************************************************************************
 */

#include "nECU_tim.h"

// static nECU_tim_Watchdog Button_Out_Watchdog = {0}, Ox_Out_Watchdog = {0};

/* Used in watchdogs */
extern Oxygen_Handle OX;
extern Button Red;

static nECU_Timer TIM_List[TIM_ID_MAX] = {0}; // List of all defined TIM
static TIM_HandleTypeDef *TIM_Handle_List[TIM_ID_MAX] = {
    [TIM_PWM_BUTTON_ID] = &htim1,
    [TIM_PWM_OX_ID] = &htim2,
    [TIM_IC_BUTTON_ID] = &htim3,
    [TIM_IC_FREQ_ID] = &htim4,
    [TIM_ADC_KNOCK_ID] = &htim8,
    [TIM_FRAME_ID] = &htim10,
}; // Lists handles for each ID
static const uint32_t TIM_ActiveChannel_Lookup[HAL_TIM_ACTIVE_CHANNEL_4 + 1] = {
    [HAL_TIM_ACTIVE_CHANNEL_1] = TIM_CHANNEL_1,
    [HAL_TIM_ACTIVE_CHANNEL_2] = TIM_CHANNEL_2,
    [HAL_TIM_ACTIVE_CHANNEL_3] = TIM_CHANNEL_3,
    [HAL_TIM_ACTIVE_CHANNEL_4] = TIM_CHANNEL_4,
}; // Lookup table to connect ACTIVE_CHANNEL with TIM_CHANNEL

uint8_t nECU_Get_FrameTimer(void) // get current value of frame timer -> Used for UART frame timestamp
{
  /* timer 11 is set to work with 0,1ms count up time, and have 8bit period*/
  bool status = false;
  if (!nECU_FlowControl_Initialize_Check(D_TIM_FRAME))
    status |= nECU_TIM_Init(TIM_FRAME_ID);
  if (!nECU_FlowControl_Working_Check(D_TIM_FRAME) && status == false)
    status |= nECU_TIM_Base_Start(TIM_FRAME_ID);
  if (status) // Break on error
    return 0;

  return (TIM_Handle_List[TIM_FRAME_ID]->Instance->CNT);
}

static nECU_TIM_ID nECU_TIM_Identify(TIM_HandleTypeDef *htim) // returns ID of given input
{
  if (htim == NULL) // break if pointer does not exist
    return TIM_ID_MAX;

  for (nECU_TIM_ID current_ID = 0; current_ID < TIM_ID_MAX; current_ID++) // search for handle
  {
    if (htim == TIM_Handle_List[current_ID]) // check if found
    {
      return current_ID;
    }
  }
  return TIM_ID_MAX; // return ID_MAX if not found
}
TIM_HandleTypeDef *nECU_TIM_getPointer(nECU_TIM_ID ID) // returns pointer to timer
{
  if (ID >= TIM_ID_MAX) // Break if invalid ID
    return NULL;

  return TIM_Handle_List[ID];
}
nECU_InputCapture *nECU_TIM_IC_getPointer(nECU_TIM_ID ID, uint32_t Channel) // pointer to IC stucture
{
  if (ID >= TIM_ID_MAX) // Break if invalid ID
    return NULL;

  if (TIM_List[ID].Channels[Channel] != TIM_Channel_IC) // Check if this is IC channel
    return NULL;

  return &TIM_List[ID].IC[Channel];
}

/* Callback functions */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  UNUSED(htim);
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  UNUSED(htim);
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim == NULL) // break if pointer does not exist
    return;

  nECU_TIM_ID current_ID = nECU_TIM_Identify(htim);
  if (current_ID >= TIM_ID_MAX) // Break if invalid ID
    return;

  nECU_TIM_IC_Callback(TIM_IC_FREQ_ID);
}

/* Used for simple time tracking */
bool nECU_TickTrack_Init(nECU_TickTrack *inst) // initialize structure
{
  if (inst == NULL)
    return true;
  inst->previousTick = HAL_GetTick();
  inst->difference = 0;
  inst->convFactor = HAL_GetTickFreq();
  return false;
}
bool nECU_TickTrack_Update(nECU_TickTrack *inst) // callback to get difference
{
  if (inst == NULL)
    return true;
  uint64_t tickNow = HAL_GetTick();
  if (tickNow < inst->previousTick) // check if data roll over
  {
    inst->difference = (tickNow + UINT32_MAX) - inst->previousTick;
  }
  else
  {
    inst->difference = tickNow - inst->previousTick;
  }
  inst->previousTick = tickNow;
  return false;
}

/* Non-blocking delay */
bool *nECU_Delay_DoneFlag(nECU_Delay *inst) // return done flag pointer of non-blocking delay
{
  return &(inst->done);
}
bool nECU_Delay_Start(nECU_Delay *inst) // start non-blocking delay
{
  if (inst == NULL)
    return true;

  bool status = false;
  status |= nECU_TickTrack_Init(&(inst->timeTrack));
  inst->timePassed = 0;
  inst->done = false;
  inst->active = true;
  return status;
}
bool nECU_Delay_Set(nECU_Delay *inst, uint32_t delay) // preset the non-blocking delay
{
  if (inst == NULL)
    return true;
  bool status = false;
  status |= nECU_TickTrack_Init(&(inst->timeTrack));
  inst->timeSet = delay / inst->timeTrack.convFactor;
  return status;
}
bool nECU_Delay_Update(nECU_Delay *inst) // update current state of non-blocking delay
{
  if (inst == NULL)
    return true;

  bool status = false;

  // it is not a time critical function, just lunch it inside main loop
  // nECU_Delay functions are based on tick setting, refer to HAL_Delay() function
  if (inst->done == false && inst->active == true) // do only if needed
  {
    status |= nECU_TickTrack_Update(&(inst->timeTrack));
    inst->timePassed += inst->timeTrack.difference;

    if (inst->timePassed > inst->timeSet)
    {
      inst->done = true;
      inst->active = false;
    }
  }
  return status;
}
bool nECU_Delay_Stop(nECU_Delay *inst) // stop non-blocking delay
{
  if (inst == NULL)
    return true;
  inst->done = false;
  inst->active = false;
  return false;
}

/* general nECU timer functions */
bool nECU_TIM_Init(nECU_TIM_ID ID) // initialize structure and precalculate variables
{
  if (ID >= TIM_ID_MAX)
    return true;

  if (nECU_FlowControl_Initialize_Check(D_TIM_PWM_BUTTON + ID)) // Check if was done
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  TIM_List[ID].refClock = TIM_CLOCK / (TIM_List[ID].htim->Init.Prescaler + 1); // add 1 for prescaler nature
  TIM_List[ID].period = 1 / TIM_List[ID].refClock;                             // calculate period of a single increment in timer

  for (uint8_t channel = 0; channel < sizeof(TIM_List[ID].Channels) / sizeof(TIM_List[ID].Channels[0]); channel++) // zero out every position of the list
  {
    TIM_List[ID].Channels[channel] = TIM_Channel_NONE;
    TIM_List[ID].IC[channel].CCR_High = 0;
    TIM_List[ID].IC[channel].CCR_Low = 0;
    TIM_List[ID].IC[channel].CCR_prev = 0;
    TIM_List[ID].IC[channel].frequency = 0;
    TIM_List[ID].IC[channel].newData = false;
    TIM_List[ID].IC[channel].Digi_Input = DigiInput_ID_MAX;
  }

  bool status = false;
  status |= !nECU_FlowControl_Initialize_Do(D_TIM_PWM_BUTTON + ID);
  if (status)
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
  return status;
}
bool nECU_TIM_PWM_Start(nECU_TIM_ID ID, uint32_t Channel) // function to start PWM on selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  bool status = false;
  if (!nECU_FlowControl_Initialize_Check(D_TIM_PWM_BUTTON + ID))
    status |= nECU_TIM_Init(D_TIM_PWM_BUTTON + ID);
  if (!nECU_FlowControl_Working_Check(D_TIM_PWM_BUTTON + ID) && status == false)
    status |= nECU_TIM_Base_Start(D_TIM_PWM_BUTTON + ID);
  if (status) // Break on error
    return status;

  // Check if this channel is already configured for something else
  if (TIM_List[ID].Channels[Channel] != TIM_Channel_PWM && TIM_List[ID].Channels[Channel] != TIM_Channel_NONE)
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  if (HAL_TIM_PWM_Start_IT(TIM_List[ID].htim, Channel) != HAL_OK) // start pwm, check result
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }
  TIM_List[ID].Channels[Channel] = TIM_Channel_PWM;

  return false;
}
bool nECU_TIM_PWM_Stop(nECU_TIM_ID ID, uint32_t Channel) // function to stop PWM on selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  // Check if this channel is already configured for something else
  if (TIM_List[ID].Channels[Channel] != TIM_Channel_PWM)
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  if (HAL_TIM_PWM_Stop_IT(TIM_List[ID].htim, Channel) != HAL_OK) // stop pwm, check result
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }
  if (HAL_TIM_GetChannelState(TIM_List[ID].htim, Channel) != HAL_TIM_CHANNEL_STATE_RESET) // confirm status of channel
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }

  TIM_List[ID].Channels[Channel] = TIM_Channel_NONE;

  nECU_TIM_Base_Stop(ID); // Stop if possible

  return false;
}
bool nECU_TIM_IC_Start(nECU_TIM_ID ID, uint32_t Channel, nECU_DigiInput_ID Digi_ID) // function to start IC on selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  if (Digi_ID > DigiInput_ID_MAX)
    return true;

  bool status = false;
  if (!nECU_FlowControl_Initialize_Check(D_TIM_PWM_BUTTON + ID))
    status |= nECU_TIM_Init(D_TIM_PWM_BUTTON + ID);
  if (!nECU_FlowControl_Working_Check(D_TIM_PWM_BUTTON + ID) && status == false)
    status |= nECU_TIM_Base_Start(D_TIM_PWM_BUTTON + ID);
  if (status) // Break on error
    return status;

  // Check if this channel is already configured for something else
  if (TIM_List[ID].Channels[Channel] != TIM_Channel_IC && TIM_List[ID].Channels[Channel] != TIM_Channel_NONE)
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  if (HAL_TIM_IC_Start_IT(TIM_List[ID].htim, Channel) != HAL_OK) // start IC, check result
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }

  TIM_List[ID].IC[Channel].Digi_Input = Digi_ID;
  if (TIM_List[ID].IC[Channel].Digi_Input > DigiInput_ID_MAX)
    TIM_List[ID].IC[Channel].Digi_Input = DigiInput_ID_MAX;
  if (!nECU_DigitalInput_Start(TIM_List[ID].IC[Channel].Digi_Input))
    return false;

  TIM_List[ID].Channels[Channel] = TIM_Channel_IC;

  // Add GPIO config

  return false;
}
bool nECU_TIM_IC_Stop(nECU_TIM_ID ID, uint32_t Channel) // function to stop IC on selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  // Check if this channel is already configured for something else
  if (TIM_List[ID].Channels[Channel] != TIM_Channel_PWM)
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  if (HAL_TIM_IC_Stop_IT(TIM_List[ID].htim, Channel) != HAL_OK) // stop ic, check result
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }
  if (HAL_TIM_GetChannelState(TIM_List[ID].htim, Channel) != HAL_TIM_CHANNEL_STATE_RESET) // confirm status of channel
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true; // indicate if not successful
  }

  if (!nECU_DigitalInput_Stop(TIM_List[ID].IC[Channel].Digi_Input))
    return false;

  TIM_List[ID].IC[Channel].Digi_Input = DigiInput_ID_MAX;
  TIM_List[ID].Channels[Channel] = TIM_Channel_NONE;

  nECU_TIM_Base_Stop(ID); // Stop if possible

  return false;
}

bool nECU_TIM_Base_Start(nECU_TIM_ID ID) // function to start base of selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  if (!nECU_FlowControl_Working_Check(D_TIM_PWM_BUTTON + ID)) // Check if was done
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  if (HAL_TIM_Base_Start_IT(TIM_List[ID].htim) != HAL_OK)
  {
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
    return true;
  }

  bool status = false;
  status |= !nECU_FlowControl_Working_Do(D_TIM_PWM_BUTTON + ID);
  if (status)
    nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
  return status;
}
bool nECU_TIM_Base_Stop(nECU_TIM_ID ID) // function to stop base of selected timer
{
  if (ID >= TIM_ID_MAX)
    return true;

  if (nECU_FlowControl_Working_Check(D_TIM_PWM_BUTTON + ID))
  {
    bool working = false;
    for (uint8_t current_Channel = 0; current_Channel < 4; current_Channel++) // Check if all channels are disabled
    {
      working |= (TIM_Channel_NONE != TIM_List[ID].Channels[current_Channel]);
    }
    if (!working)
    {
      if (HAL_TIM_Base_Stop_IT(TIM_List[ID].htim) == HAL_OK)
      {
        nECU_FlowControl_Stop_Do(D_TIM_PWM_BUTTON + ID);
      }
      else
      {
        nECU_FlowControl_Error_Do(D_TIM_PWM_BUTTON + ID);
        return true;
      }
    }
  }
  return false;
}

static bool nECU_TIM_IC_Callback(nECU_TIM_ID ID) // callback function to calculate basic parameters
{
  if (ID >= TIM_ID_MAX)
    return true;

  uint32_t channel = TIM_ActiveChannel_Lookup[TIM_List[ID].htim->Channel];
  uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(TIM_List[ID].htim, channel);
  uint8_t channel_ic = (TIM_List[ID].htim->Channel / 4) + 1;

  /* Calculate difference */
  uint16_t Difference = 0; // in Timer pulses

  if (TIM_List[ID].IC[channel_ic].CCR_prev > CurrentCCR)
    Difference = ((TIM_List[ID].htim->Init.Period + 1) - TIM_List[ID].IC[channel_ic].CCR_prev) + CurrentCCR;
  else
    Difference = CurrentCCR - TIM_List[ID].IC[channel_ic].CCR_prev;

  /* Debounce */
  if (Difference < 10) // Pulse has to be at least 10x longer then channel period
    return false;

  nECU_DigitalInput_Routine(TIM_List[ID].IC[channel_ic].Digi_Input);

  /* If GPIO was defined: Detect edge and calculate times */
  if (TIM_List[ID].IC[channel_ic].Digi_Input < DigiInput_ID_MAX)
  {
    if (nECU_DigitalInput_getValue(TIM_List[ID].IC[channel_ic].Digi_Input))
      TIM_List[ID].IC[channel_ic].CCR_Low = Difference;
    else
      TIM_List[ID].IC[channel_ic].CCR_High = Difference;

    TIM_List[ID].IC[channel_ic].frequency = nECU_FloatToUint(TIM_List[ID].refClock / (TIM_List[ID].IC[channel_ic].CCR_High + TIM_List[ID].IC[channel_ic].CCR_Low), 16);
  }
  else
    TIM_List[ID].IC[channel_ic].frequency = nECU_FloatToUint(TIM_List[ID].refClock / (Difference * 2), 16);

  TIM_List[ID].IC[channel_ic].CCR_prev = CurrentCCR;
  TIM_List[ID].IC[channel_ic].newData = true;
  return false;
}

// /* Watchdog for timers detection */
// bool nECU_tim_Watchdog_Init(void) // initialize structure
// {
//   // Button out timer
//   nECU_tim_Watchdog_Init_struct(&Button_Out_Watchdog);
//   Button_Out_Watchdog.tim = &(Red.light.Timer);

//   // Ox out timer
//   nECU_tim_Watchdog_Init_struct(&Ox_Out_Watchdog);
//   Ox_Out_Watchdog.tim = &(OX.Heater);
// }
// bool nECU_tim_Watchdog_Init_struct(nECU_tim_Watchdog *watchdog) // set default values to variables
// {
//   watchdog->error = false;
//   watchdog->warning = false;
//   nECU_TickTrack_Init(&(watchdog->timeTrack));
//   watchdog->counter_max = watchdog->tim->period * (watchdog->tim->htim->Init.Period + 1) * WATCHDOG_PERIOD_MULTIPLIER;
// }

// void nECU_tim_Watchdog_Periodic(void) // watchdog function for active timers
// {
//   /* Get time difference of function calls */

//   /* PWM Outputs */
//   nECU_tim_Watchdog_CheckStates(&Button_Out_Watchdog);   // state of peripheral and outputs
//   nECU_tim_Watchdog_updateCounter(&Button_Out_Watchdog); // update watchdog
//   nECU_tim_Watchdog_CheckCounter(&Button_Out_Watchdog);  // check if counter in bounds

//   nECU_tim_Watchdog_CheckStates(&Ox_Out_Watchdog);   // state of peripheral and outputs
//   nECU_tim_Watchdog_updateCounter(&Ox_Out_Watchdog); // update watchdog
//   nECU_tim_Watchdog_CheckCounter(&Ox_Out_Watchdog);  // check if counter in bounds
// }
// bool nECU_tim_Watchdog_updateCounter(nECU_tim_Watchdog *watchdog) // update counter value based on systick
// {
//   nECU_TickTrack_Update(&(watchdog->timeTrack)); // update tracker (get tick difference)

//   watchdog->counter_ms += watchdog->timeTrack.difference * watchdog->timeTrack.convFactor; // add diference as time
// }
// bool nECU_tim_Watchdog_Callback(TIM_HandleTypeDef *htim) // function to be called on timer interrupt
// {
//   if (htim == NULL)
//     return true;

//   nECU_tim_Watchdog *inst;

//   /* find which watchdog is responsible */
//   if (htim == Button_Out_Watchdog.tim->htim)
//   {
//     inst = &Button_Out_Watchdog;
//   }
//   else if (htim == Ox_Out_Watchdog.tim->htim)
//   {
//     inst = &Ox_Out_Watchdog;
//   }

//   /* perform watchdog update */
//   inst->counter_ms = 0;
//   nECU_TickTrack_Init(&(inst->timeTrack)); // clear variables
//   return false;
// }

// static bool nECU_tim_Watchdog_CheckStates(nECU_tim_Watchdog *watchdog) // check state of peripheral
// {
//   if (watchdog == NULL)
//     return true;

//   if (watchdog->tim->htim->State == HAL_TIM_STATE_RESET) // check if peripheral in use
//   {
//     return false; // peripheral not initialized
//   }
//   if (watchdog->tim->htim->State == HAL_TIM_STATE_ERROR) // chcek if peripheral error
//   {
//     watchdog->error = true;
//     return true; // peripheral error
//   }
//   if (nECU_tim_Watchdog_CheckChannels(watchdog->tim))
//   {
//     watchdog->error = true;
//     return true; // channel error
//   }
//   return false; // timer is OK
// }
// static bool nECU_tim_Watchdog_CheckCounter(nECU_tim_Watchdog *watchdog) // check counter, determine timer error
// {
//   if (watchdog->counter_max < watchdog->counter_ms)
//   {
//     watchdog->error = true;
//     return true;
//   }
//   return false;
// }
// static bool nECU_tim_Watchdog_CheckChannels(nECU_Timer *tim) // check channels of timer
// {
//   if (tim == NULL)
//     return false;

//   for (uint8_t i = 0; i < tim->Channel_Count; i++)
//   {
//     if (HAL_TIM_GetChannelState(tim->htim, tim->Channel_List[i]))
//     {
//       return true;
//     }
//   }
//   return false;
// }
