/**
 ******************************************************************************
 * @file    nECU_button.c
 * @brief   This file provides code for button inputs and their backlight.
 ******************************************************************************
 */

#include "nECU_button.h"

static Button Red = {0};
static Button Orange = {0};
static Button Green = {0};

extern nECU_ProgramBlockData D_Button_Red, D_Button_Orange, D_Button_Green; // diagnostic and flow control data

/* All button functions */
bool Button_Start(void)
{
  bool status = false;
  if (D_Button_Red.Status == D_BLOCK_STOP)
  {
    bool status_Red = false;
    status_Red |= ButtonLight_Init(&Red.light, 1, &BUTTON_OUTPUT_TIMER);
    status_Red |= ButtonInput_Init(&Red.input, 1, &BUTTON_INPUT_TIMER);
    if (!status_Red)
    {
      D_Button_Red.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
    status |= status_Red;
  }
  if (D_Button_Orange.Status == D_BLOCK_STOP)
  {
    bool status_Orange = false;
    status_Orange |= ButtonLight_Init(&Orange.light, 2, &BUTTON_OUTPUT_TIMER);
    status_Orange |= ButtonInput_Init(&Orange.input, 2, &BUTTON_INPUT_TIMER);
    if (!status_Orange)
    {
      D_Button_Orange.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
    status |= status_Orange;
  }
  if (D_Button_Green.Status == D_BLOCK_STOP)
  {
    bool status_Green = false;
    status_Green |= ButtonLight_Init(&Green.light, 3, &BUTTON_OUTPUT_TIMER);
    status_Green |= ButtonInput_Init(&Green.input, 3, &BUTTON_INPUT_TIMER);
    if (!status_Green)
    {
      D_Button_Green.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
    status |= status_Green;
  }
  return status;
}
void Button_Stop(void)
{
  // Stop lights
  ButtonLight_Stop(&Red.light);
  ButtonLight_Stop(&Orange.light);
  ButtonLight_Stop(&Green.light);

  // Stop inputs
  ButtonInput_Stop(&Red.input);
  ButtonInput_Stop(&Orange.input);
  ButtonInput_Stop(&Green.input);

  D_Button_Red.Status -= D_BLOCK_INITIALIZED_WORKING;
  D_Button_Orange.Status -= D_BLOCK_INITIALIZED_WORKING;
  D_Button_Green.Status -= D_BLOCK_INITIALIZED_WORKING;
}

/* BUTTON LIGHT BEGIN */
static bool ButtonLight_Init(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonLight object with corresponding timer
{
  bool status = false;

  Light->Timer.htim = htim;
  nECU_tim_Init_struct(&Light->Timer);
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
  status |= (nECU_tim_PWM_start(&Light->Timer) != TIM_OK);

  nECU_TickTrack_Init(&(Light->TimeTracker));

  Light->Mode = BUTTON_MODE_OFF; // Turn off

  return status;
}
static void ButtonLight_Update(ButtonLight *Light) // periodic animation update function
{
  ButtonLight_TimeTrack(Light);
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
static void ButtonLight_TimeTrack(ButtonLight *Light) // funtion called to update time passed
{
  nECU_TickTrack_Update(&(Light->TimeTracker));                                 // update tracker
  Light->Time += Light->TimeTracker.difference * Light->TimeTracker.convFactor; // add to time elapsed
}
static void ButtonLight_Stop(ButtonLight *Light) // stops PWM for seected button
{
  nECU_tim_PWM_stop(&Light->Timer);
}

void ButtonLight_UpdateAll(void) // function to launch updates for all buttons
{
  if (D_Button_Red.Status & D_BLOCK_WORKING)
  {
    ButtonLight_Update(&Red.light);
    nECU_Debug_ProgramBlockData_Update(&D_Button_Red);
  }
  if (D_Button_Orange.Status & D_BLOCK_WORKING)
  {
    ButtonLight_Update(&Orange.light);
    nECU_Debug_ProgramBlockData_Update(&D_Button_Orange);
  }
  if (D_Button_Green.Status & D_BLOCK_WORKING)
  {
    ButtonLight_Update(&Green.light);
    nECU_Debug_ProgramBlockData_Update(&D_Button_Green);
  }
}
/* BUTTON LIGHT END */

/* BUTTON INPUT BEGIN */
static bool ButtonInput_Init(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonInput object with corresponding timer and GPIO
{
  bool status = false;
  button->Timer.htim = htim;
  nECU_tim_Init_struct(&button->Timer);
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
  status |= (nECU_tim_IC_start(&button->Timer) != TIM_OK);

  return status;
}
static void ButtonInput_Stop(ButtonInput *button) // stop Input Capture for selected button
{
  nECU_tim_IC_stop(&button->Timer);
}

void ButtonInput_Identify(TIM_HandleTypeDef *htim) // function to identify to which button is pressed
{
  // Find correct button input by channel interrupted
  ButtonInput *temporary;
  nECU_ProgramBlock_Status *status;
  if (htim->Channel == Red.input.Channel_IC)
  {
    temporary = &Red.input;
    status = &D_Button_Red.Status;
  }
  else if (htim->Channel == Orange.input.Channel_IC)
  {
    temporary = &Orange.input;
    status = &D_Button_Orange.Status;
  }
  else if (htim->Channel == Green.input.Channel_IC)
  {
    temporary = &Green.input;
    status = &D_Button_Green.Status;
  }
  else
  {
    return;
  }

  if (*status & D_BLOCK_WORKING)
  {
    ButtonInput_InterruptRoutine(temporary);
  }
  else
  {
    *status = D_BLOCK_CODE_ERROR;
  }
}
static void ButtonInput_InterruptRoutine(ButtonInput *button) // routine to be called after input capture callback (updates button structure)
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
Button_ClickType ButtonInput_GetType(Button_ID id) // get click type if avaliable
{
  // Find correct light by ID
  ButtonInput *inst;
  nECU_ProgramBlock_Status *status;
  switch (id)
  {
  case RED_BUTTON_ID:
    inst = &Red.input;
    status = &D_Button_Red.Status;
    break;
  case ORANGE_BUTTON_ID:
    inst = &Orange.input;
    status = &D_Button_Orange.Status;
    break;
  case GREEN_BUTTON_ID:
    inst = &Green.input;
    status = &D_Button_Green.Status;
    break;
  default:
    D_Button_Red.Status = D_BLOCK_CODE_ERROR;
    D_Button_Orange.Status = D_BLOCK_CODE_ERROR;
    D_Button_Green.Status = D_BLOCK_CODE_ERROR;
    return 0;
  }

  if (!(*status & D_BLOCK_INITIALIZED_WORKING))
  {
    return 0;
  }

  // Copy click type to output
  if (inst->newType == true)
  {
    inst->newType = false;
    return (inst->Type);
  }
  return CLICK_TYPE_NONE;
}
/* BUTTON INPUT END */

/* Animations */
static bool ButtonLight_Identify(Button_ID id, ButtonLight *light) // find corresponding light structrue, return if ok
{
  // Find correct light by ID
  nECU_ProgramBlock_Status *status;
  switch (id)
  {
  case RED_BUTTON_ID:
    light = &Red.light;
    status = &D_Button_Red.Status;
    break;
  case ORANGE_BUTTON_ID:
    light = &Orange.light;
    status = &D_Button_Orange.Status;
  case GREEN_BUTTON_ID:
    light = &Green.light;
    status = &D_Button_Green.Status;
  default:
    D_Button_Red.Status = D_BLOCK_CODE_ERROR;
    D_Button_Orange.Status = D_BLOCK_CODE_ERROR;
    D_Button_Green.Status = D_BLOCK_CODE_ERROR;
    return false;
  }

  if (!(*status & D_BLOCK_INITIALIZED_WORKING))
  {
    *status = D_BLOCK_ERROR;
    return false;
  }

  return true;
}

void ButtonLight_SetOne(Button_ID id, bool state) // set selected button
{
  uint8_t Mode = BUTTON_MODE_RESTING; // By default turn off
  if (state != false)
  {
    Mode = BUTTON_MODE_ON;
  }

  ButtonLight *temporary;

  if (ButtonLight_Identify(id, temporary)) // do only if button exists and is working
  {
    /* Perform operation */
    if (temporary->Mode != BUTTON_MODE_ANIMATED)
    {
      temporary->Mode = Mode;
      temporary->ModePrev = BUTTON_MODE_OFF;
    }
  }
}
void ButtonLight_Breath(Button_ID id, uint8_t Speed, uint16_t Count) // breath one button
{
  ButtonLight *temporary;

  if (ButtonLight_Identify(id, temporary)) // do only if button exists and is working
  {
    /* Perform operation */
    temporary->Breathing.speed = Speed;
    temporary->Breathing.count = Count;
    temporary->Mode = BUTTON_MODE_ANIMATED;
  }
}
void ButtonLight_Blink(Button_ID id, uint8_t Speed, uint16_t Count) // blink one button
{
  ButtonLight *temporary;

  if (ButtonLight_Identify(id, temporary)) // do only if button exists and is working
  {
    /* Perform operation */
    temporary->Blinking.speed = Speed;
    temporary->Blinking.count = Count;
    temporary->Mode = BUTTON_MODE_ANIMATED;
  }
}