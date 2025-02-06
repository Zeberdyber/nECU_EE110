/**
 ******************************************************************************
 * @file    nECU_button.c
 * @brief   This file provides code for button inputs and their backlight.
 ******************************************************************************
 */

#include "nECU_button.h"

static Button Button_List[BUTTON_ID_MAX] = {0};

/* All button functions */
bool nECU_Button_Start(void)
{
  bool status = false;

  for (uint8_t Current_ID = 0; Current_ID < BUTTON_ID_MAX; Current_ID++)
  {
    status |= nECU_Button_Start_Single(Current_ID);
  }

  return status;
}
bool nECU_Button_Stop(void)
{
  bool status = false;
  for (uint8_t Current_ID = 0; Current_ID < BUTTON_ID_MAX; Current_ID++)
  {
    bool current_status = false;
    if (!nECU_FlowControl_Stop_Check(D_Button_Red + Current_ID))
    {
      nECU_Button_Light_Stop(&Button_List[Current_ID].light);
      nECU_Button_Input_Stop(&Button_List[Current_ID].input);
      current_status |= !nECU_FlowControl_Stop_Do(D_Button_Red + Current_ID);
    }
    if (current_status)
    {
      nECU_FlowControl_Error_Do(D_Button_Red + Current_ID);
    }
    status |= current_status;
  }
  return status;
}

static bool nECU_Button_Start_Single(Button_ID ID) // Perform start on single button
{
  if (ID >= EGT_ID_MAX)
    return true;

  bool status = false;
  if (!nECU_FlowControl_Initialize_Check(D_Button_Red + ID) && status == false)
  {
    status |= nECU_Button_Light_Start(&Button_List[ID].light, 1 + ID, &BUTTON_OUTPUT_TIMER);
    status |= nECU_Button_Input_Start(&Button_List[ID].input, 1 + ID, &BUTTON_INPUT_TIMER);

    if (status == false)
    {
      status |= !nECU_FlowControl_Initialize_Do(D_Button_Red + ID);
    }
  }
  if (!nECU_FlowControl_Working_Check(D_Button_Red + ID) && (status == false))
  {
    status |= !nECU_FlowControl_Working_Do(D_Button_Red + ID);
  }
  if (status)
  {
    nECU_FlowControl_Error_Do(D_Button_Red + ID);
  }
  return status;
}
static bool nECU_Button_Stop_Single(Button_ID ID) // Perform stop on single button
{
  if (ID >= EGT_ID_MAX)
    return true;
}
/* BUTTON LIGHT BEGIN */
static bool nECU_Button_Light_Start(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonLight object with corresponding timer
{
  bool status = false;

  Light->Timer.htim = htim;
  nECU_TIM_Init(&Light->Timer);
  Light->Timer.Channel_Count = 1;
  Light->CCR = 0;

  switch (Channel)
  {
  case 1:
    Light->Timer.htim->Instance->CCR1 = 0;
    Light->Timer.Channel_List[0] = TIM_CHANNEL_1;
    break;
  case 2:
    Light->Timer.htim->Instance->CCR2 = 0;
    Light->Timer.Channel_List[0] = TIM_CHANNEL_2;
    break;
  case 3:
    Light->Timer.htim->Instance->CCR3 = 0;
    Light->Timer.Channel_List[0] = TIM_CHANNEL_3;
    break;

  default:
    status |= true;
    return status;
  }
  status |= (nECU_TIM_PWM_Start(&Light->Timer) != TIM_OK);

  nECU_TickTrack_Init(&(Light->TimeTracker));

  Light->Mode = BUTTON_MODE_OFF; // Turn off

  return status;
}
static void nECU_Button_Light_Routine(ButtonLight *Light) // periodic animation update function
{
  nECU_Button_Light_TimeTrack(Light);
  switch (Light->Mode)
  {
  case BUTTON_MODE_OFF:
    Light->Brightness = 0;
    break;
  case BUTTON_MODE_RESTING:
    Light->Brightness = BUTTON_LIGHT_RESTING_BRIGHNESS;
    break;
  case BUTTON_MODE_GO_TO_REST:
    if (Light->Mode != Light->ModePrev)
    {
      Light->ModePrev = Light->Mode;
      Light->Time = 0;
    }

    if (Light->Time < BUTTON_LIGHT_TIME_TO_REST / 2)
    {
      Light->Brightness = BUTTON_LIGHT_MAXIMUM_INFILL;
    }
    else if (Light->Time >= BUTTON_LIGHT_TIME_TO_REST)
    {
      Light->Mode = BUTTON_MODE_RESTING;
    }
    else
    { // Simple linear equation (right, it took me an hour to figure t out)
      Light->Brightness = BUTTON_LIGHT_MAXIMUM_INFILL - (BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_RESTING_BRIGHNESS) * (Light->Time - (BUTTON_LIGHT_TIME_TO_REST / 2)) / (BUTTON_LIGHT_TIME_TO_REST / 2);
    }
    break;
  case BUTTON_MODE_ANIMATED:
    if (Light->Mode != Light->ModePrev)
    {
      Light->ModePrev = Light->Mode;
      Light->Time = 0;
    }
    if (Light->Breathing.count > 0)
    {
      Light->Breathing.state = ((uint8_t)(((Light->Time / BUTTON_LIGHT_BREATHING_SLOWDOWN) * Light->Breathing.speed) / 100) % 255) - INT8_MAX;

      if (Light->Breathing.state != Light->Breathing.prevState)
      {
        if (Light->Breathing.state == 0 && Light->Breathing.prevState < 0)
        {
          Light->Breathing.count--;
        }
        Light->Breathing.prevState = Light->Breathing.state;
      }
      if (Light->Breathing.state < 0) // flip the state
      {
        Light->Breathing.state = -Light->Breathing.state;
      }

      Light->Brightness = (((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * (Light->Breathing.state + 1)) / INT8_MAX) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->Blinking.count > 0)
    {
      Light->Blinking.state = (uint8_t)((((Light->Time / BUTTON_LIGHT_BLINKING_SLOWDOWN) * Light->Blinking.speed) / 100)) % 2;
      if (Light->Blinking.state != Light->Blinking.prevState) // Execute only on positive edge
      {
        if (Light->Blinking.state == 0 && Light->Blinking.prevState == 1)
        {
          Light->Blinking.count--;
        }
        Light->Blinking.prevState = Light->Blinking.state;
      }
      Light->Brightness = ((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * Light->Blinking.state) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->Breathing.count + Light->Blinking.count == 0)
    {
      Light->Blinking.state = 0;
      Light->Blinking.prevState = 0;
      Light->Breathing.state = 0;
      Light->Breathing.prevState = 0;
      Light->Mode = BUTTON_MODE_GO_TO_REST;
    }
    break;
  case BUTTON_MODE_ON:
    Light->Brightness = BUTTON_LIGHT_MAXIMUM_INFILL;
    break;
  default:
    break;
  }

  Light->CCR = (Light->Brightness / 100) * Light->Timer.htim->Init.Period;

  switch (Light->Timer.Channel_List[0])
  {
  case TIM_CHANNEL_1:
    Light->Timer.htim->Instance->CCR1 = Light->CCR;
    break;
  case TIM_CHANNEL_2:
    Light->Timer.htim->Instance->CCR2 = Light->CCR;
    break;
  case TIM_CHANNEL_3:
    Light->Timer.htim->Instance->CCR3 = Light->CCR;
    break;

  default:
    break;
  }
}
static void nECU_Button_Light_TimeTrack(ButtonLight *Light) // funtion called to update time passed
{
  nECU_TickTrack_Update(&(Light->TimeTracker));                                 // update tracker
  Light->Time += Light->TimeTracker.difference * Light->TimeTracker.convFactor; // add to time elapsed
}
static void nECU_Button_Light_Stop(ButtonLight *Light) // stops PWM for seected button
{
  nECU_TIM_PWM_Stop(&Light->Timer);
}
void nECU_Button_Light_Routine_All(void) // function to launch updates for all buttons
{
  for (uint8_t Current_ID = 0; Current_ID < BUTTON_ID_MAX; Current_ID++)
  {
    if (!nECU_FlowControl_Working_Check(D_Button_Red + Current_ID))
    {
      nECU_Button_Light_Routine(&Button_List[Current_ID].light);
      nECU_Debug_ProgramBlockData_Update(D_Button_Red + Current_ID);
    }
  }
}
/* BUTTON LIGHT END */

/* BUTTON INPUT BEGIN */
static bool nECU_Button_Input_Start(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonInput object with corresponding timer and GPIO
{
  bool status = false;
  button->Timer.htim = htim;
  nECU_TIM_Init(&button->Timer);
  button->Timer.Channel_Count = 1;

  button->RisingCCR = 0;
  button->buttonPin.GPIOx = GPIOC;

  switch (Channel)
  {
  case 1:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_1;
    button->Timer.Channel_List[0] = TIM_CHANNEL_1;
    button->buttonPin.GPIO_Pin = B1_S_Pin;
    break;
  case 2:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_2;
    button->Timer.Channel_List[0] = TIM_CHANNEL_2;
    button->buttonPin.GPIO_Pin = B2_S_Pin;
    break;
  case 3:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_3;
    button->Timer.Channel_List[0] = TIM_CHANNEL_3;
    button->buttonPin.GPIO_Pin = B3_S_Pin;
    break;

  default:
    status |= true;
    return status;
  }
  status |= (nECU_TIM_IC_Start(&button->Timer) != TIM_OK);

  return status;
}
static void nECU_Button_Input_Stop(ButtonInput *button) // stop Input Capture for selected button
{
  nECU_TIM_IC_Stop(&button->Timer);
}
void nECU_Button_Input_Identify(TIM_HandleTypeDef *htim) // function to identify to which button is pressed
{
  Button_ID current_ID = htim->Channel - 1;

  if (current_ID >= BUTTON_ID_MAX) // Break if this button ID is not valid
    return;

  if (!nECU_FlowControl_Working_Check(D_Button_Red + current_ID)) // Break if not working
  {
    nECU_FlowControl_Error_Do(D_Button_Red + current_ID); // Report error
    return;
  }

  nECU_Button_Input_InterruptRoutine(&Button_List[current_ID].input);
}
static void nECU_Button_Input_InterruptRoutine(ButtonInput *button) // routine to be called after input capture callback (updates button structure)
{
  uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(button->Timer.htim, button->Timer.Channel_List[0]);

  /* Calculate difference */
  uint32_t Difference = 0;
  if (button->RisingCCR > CurrentCCR)
  {
    Difference = ((button->Timer.htim->Init.Period - button->RisingCCR) + CurrentCCR) * 1000 / button->Timer.refClock;
  }
  else
  {
    Difference = (CurrentCCR - button->RisingCCR) * 1000 / button->Timer.refClock;
  }

  if (Difference < BUTTON_INPUT_MINIMUM_PULSE_TIME)
  {
    button->RisingCCR = CurrentCCR;
    return;
  }

  button->buttonPin.State = HAL_GPIO_ReadPin(button->buttonPin.GPIOx, button->buttonPin.GPIO_Pin);

  if (button->buttonPin.State == (GPIO_PinState)SET) // Rising edge
  {
    if (Difference < BUTTON_INPUT_DOUBLE_CLICK_TIME)
    {
      /* double click */
      button->Type = CLICK_TYPE_DOUBLE_CLICK;
      button->newType = true;
    }
    else
    {
      button->RisingCCR = CurrentCCR;
    }
  }
  else // Falling edge
  {
    if (Difference > BUTTON_INPUT_HOLD_TIME)
    {
      /* hold */
      button->Type = CLICK_TYPE_HOLD;
      button->newType = true;
    }
    else if (Difference < BUTTON_INPUT_DOUBLE_CLICK_TIME && button->newType == false)
    {
      /* single click */
      button->Type = CLICK_TYPE_SINGLE_CLICK;
      button->newType = true;
    }
  }
}
Button_ClickType nECU_Button_Input_GetType(Button_ID ID) // get click type if avaliable
{
  if (ID >= BUTTON_ID_MAX)
    return CLICK_TYPE_NONE;

  if (!nECU_FlowControl_Working_Check(D_Button_Red + ID)) // Break if not working
  {
    nECU_FlowControl_Error_Do(D_Button_Red + ID); // Report error
    return CLICK_TYPE_NONE;
  }

  // Copy click type to output if new state detected
  if (Button_List[ID].input.newType == true)
  {
    Button_List[ID].input.newType = false;
    return (Button_List[ID].input.Type);
  }
  return CLICK_TYPE_NONE;
}
/* BUTTON INPUT END */

/* Animations */
static bool nECU_Button_Light_Identify(Button_ID ID, ButtonLight **light) // find corresponding light structrue, return if ok
{
  if (ID >= BUTTON_ID_MAX)
    return false;

  if (!nECU_FlowControl_Working_Check(D_Button_Red + ID)) // Break if not working
  {
    nECU_FlowControl_Error_Do(D_Button_Red + ID); // Report error
    *light = NULL;
    return false;
  }

  *light = &Button_List[ID].light;
  return true;
}
void nECU_Button_Light_SetOne(Button_ID ID, bool state) // set selected button
{
  if (ID >= BUTTON_ID_MAX)
    return;

  uint8_t Mode = BUTTON_MODE_RESTING; // By default turn off
  if (state != false)
  {
    Mode = BUTTON_MODE_ON;
  }

  ButtonLight *temporary;

  if (nECU_Button_Light_Identify(ID, &temporary)) // do only if button exists and is working
  {
    if (temporary) // Check if pointer exists
    {
      /* Perform operation */
      if (temporary->Mode != BUTTON_MODE_ANIMATED)
      {
        temporary->Mode = Mode;
        temporary->ModePrev = BUTTON_MODE_OFF;
      }
    }
  }
}
void nECU_Button_Light_Breath(Button_ID ID, uint8_t Speed, uint16_t Count) // breath one button
{
  if (ID >= BUTTON_ID_MAX)
    return;

  ButtonLight *temporary;

  if (nECU_Button_Light_Identify(ID, &temporary)) // do only if button exists and is working
  {
    if (temporary) // Check if pointer exists
    {
      /* Perform operation */
      temporary->Breathing.speed = Speed;
      temporary->Breathing.count = Count;
      temporary->Mode = BUTTON_MODE_ANIMATED;
    }
  }
}
void nECU_Button_Light_Blink(Button_ID ID, uint8_t Speed, uint16_t Count) // blink one button
{
  if (ID >= BUTTON_ID_MAX)
    return;

  ButtonLight *temporary;

  if (nECU_Button_Light_Identify(ID, &temporary)) // do only if button exists and is working
  {
    if (temporary) // Check if pointer exists
    {
      /* Perform operation */
      temporary->Blinking.speed = Speed;
      temporary->Blinking.count = Count;
      temporary->Mode = BUTTON_MODE_ANIMATED;
    }
  }
}