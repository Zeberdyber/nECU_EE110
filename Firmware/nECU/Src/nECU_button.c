/**
 ******************************************************************************
 * @file    nECU_button.c
 * @brief   This file provides code for backlight buttons.
 ******************************************************************************
 */

#include "nECU_button.h"

Button Red;
Button Orange;
Button Green;

/* All button functions */
void Button_Start(void)
{
  ButtonLight_Init(&Red.light, 1, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Red.input, 1, &BUTTON_INPUT_TIMER);

  ButtonLight_Init(&Orange.light, 2, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Orange.input, 2, &BUTTON_INPUT_TIMER);

  ButtonLight_Init(&Green.light, 3, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Green.input, 3, &BUTTON_INPUT_TIMER);
}
void Button_Stop(void)
{
  // Stop lights
  ButtonLight_Stop(&Red.light);
  ButtonLight_Stop(&Orange.light);
  ButtonLight_Stop(&Green.light);
  ButtonLight_Stop_Timer(&Red.light); // Any button passed to the function

  // Stop inputs
  ButtonInput_Stop(&Red.input);
  ButtonInput_Stop(&Orange.input);
  ButtonInput_Stop(&Green.input);
  ButtonInput_Stop_Timer(&Red.input); // Any button passed to the function
}

/* BUTTON LIGHT BEGIN */
void ButtonLight_Init(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonLight object with corresponding timer
{
  Light->Timer = htim;

  HAL_TIM_Base_Start_IT(Light->Timer);
  switch (Channel)
  {
  case 1:
    Light->Timer->Instance->CCR1 = 0;
    HAL_TIM_PWM_Start_IT(Light->Timer, TIM_CHANNEL_1);
    Light->Channel = 1;
    Light->CCR = 0;
    break;
  case 2:
    Light->Timer->Instance->CCR2 = 0;
    HAL_TIM_PWM_Start_IT(Light->Timer, TIM_CHANNEL_2);
    Light->Channel = 2;
    Light->CCR = 0;
    break;
  case 3:
    Light->Timer->Instance->CCR3 = 0;
    HAL_TIM_PWM_Start_IT(Light->Timer, TIM_CHANNEL_3);
    Light->Channel = 3;
    Light->CCR = 0;
    break;

  default:
    break;
  }

  Light->Mode = BUTTON_MODE_OFF; // Turn off
  Light->UpdateInterval = (float)1000 / (TIM_CLOCK / ((Light->Timer->Init.Prescaler + 1) * (Light->Timer->Init.Period + 1)));
}
void ButtonLight_Update(ButtonLight *Light) // periodic animation update function
{
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
    if (Light->BreathingCount > 0)
    {
      Light->BreathingState = ((uint16_t)(Light->Time * Light->UpdateInterval * BUTTON_LIGHT_BREATHING_SPEEDUP / Light->BreathingSpeed) % 255) - 128;

      if (Light->BreathingState != Light->BreathingStatePrev)
      {
        if (Light->BreathingState < 0 && Light->BreathingStatePrev > 0)
        {
          Light->BreathingCount--;
        }
        Light->BreathingStatePrev = Light->BreathingState;
      }

      if (Light->BreathingState < 0)
      {
        Light->BreathingState = -Light->BreathingState;
      }

      Light->Brightness = (((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * Light->BreathingState) / 128) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->BlinkingCount > 0)
    {
      Light->BlinkingState = (uint32_t)(Light->Time * Light->UpdateInterval / (Light->BlinkingSpeed * BUTTON_LIGHT_BLINKING_SLOWDOWN)) % 2;
      if (Light->BlinkingState != Light->BlinkingStatePrev) // Execute only on positive edge
      {
        Light->BlinkingStatePrev = Light->BlinkingState;
        if (Light->BlinkingState == 1)
        {
          Light->BlinkingCount--;
        }
      }
      Light->Brightness = ((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * Light->BlinkingState) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->BreathingCount + Light->BlinkingCount == 0)
    {
      Light->BlinkingState = 0;
      Light->BlinkingStatePrev = 0;
      Light->Mode = BUTTON_MODE_GO_TO_REST;
    }
    break;
  case BUTTON_MODE_ON:
    Light->Brightness = BUTTON_LIGHT_MAXIMUM_INFILL;
    break;
  default:
    break;
  }

  Light->CCR = Light->Brightness * (Light->Timer->Init.Period / 100);

  switch (Light->Channel)
  {
  case 1:
    Light->Timer->Instance->CCR1 = Light->CCR;
    break;
  case 2:
    Light->Timer->Instance->CCR2 = Light->CCR;
    break;
  case 3:
    Light->Timer->Instance->CCR3 = Light->CCR;
    break;

  default:
    break;
  }
}
void ButtonLight_Set_Blink(ButtonLight *Light, uint8_t Speed, uint16_t Count) // setup blinking animation
{
  Light->BlinkingSpeed = Speed;
  Light->BlinkingCount = Count;
  Light->Mode = BUTTON_MODE_ANIMATED;
}
void ButtonLight_Set_Breathe(ButtonLight *Light, uint8_t Speed, uint16_t Count) // setup breathing animation
{
  Light->BreathingSpeed = Speed;
  Light->BreathingCount = Count;
  Light->Mode = BUTTON_MODE_ANIMATED;
}
void ButtonLight_Set_Mode(ButtonLight *Light, ButtonLight_Mode Mode) // setup mode (animation type)
{
  Light->Mode = Mode;
  Light->ModePrev = BUTTON_MODE_OFF;
}
void ButtonLight_UpdateAll(void) // function to launch updates for all buttons
{
  ButtonLight_Update(&Red.light);
  ButtonLight_Update(&Orange.light);
  ButtonLight_Update(&Green.light);
}
void ButtonLight_TimingEvent(void) // funtion called after pulse finished interrupt from PWM timer
{
  Red.light.Time += Red.light.UpdateInterval;
  Orange.light.Time += Orange.light.UpdateInterval;
  Green.light.Time += Green.light.UpdateInterval;
}
void ButtonLight_Stop(ButtonLight *Light) // stops PWM for seected button
{
  switch (Light->Channel)
  {
  case 1:
    Light->Timer->Instance->CCR1 = 0;
    HAL_TIM_PWM_Stop_IT(Light->Timer, TIM_CHANNEL_1);
    break;
  case 2:
    Light->Timer->Instance->CCR2 = 0;
    HAL_TIM_PWM_Stop_IT(Light->Timer, TIM_CHANNEL_2);
    break;
  case 3:
    Light->Timer->Instance->CCR3 = 0;
    HAL_TIM_PWM_Stop_IT(Light->Timer, TIM_CHANNEL_3);
    break;

  default:
    break;
  }
}
void ButtonLight_Stop_Timer(ButtonLight *Light) // stops timer base
{
  HAL_TIM_Base_Stop_IT(Light->Timer);
}
/* BUTTON LIGHT END */

/* BUTTON INPUT BEGIN */
void ButtonInput_Init(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonInput object with corresponding timer and GPIO
{
  button->Timer = htim;
  button->RisingCCR = 0;
  button->refClock = TIM_CLOCK / (button->Timer->Init.Prescaler + 1);
  button->GPIOx = GPIOC;

  HAL_TIM_Base_Start_IT(button->Timer);

  switch (Channel)
  {
  case 1:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_1;
    button->Channel = TIM_CHANNEL_1;
    button->GPIO_Pin = B1_S_Pin;
    break;
  case 2:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_2;
    button->Channel = TIM_CHANNEL_2;
    button->GPIO_Pin = B2_S_Pin;
    break;
  case 3:
    button->Channel_IC = HAL_TIM_ACTIVE_CHANNEL_3;
    button->Channel = TIM_CHANNEL_3;
    button->GPIO_Pin = B3_S_Pin;

    break;

  default:
    break;
  }

  HAL_TIM_IC_Start_IT(button->Timer, button->Channel);
}
void ButtonInput_TimingEvent(TIM_HandleTypeDef *htim) // funtion called after input capture interrupt from timer
{
  ButtonInput_Identify(htim);
}
void ButtonInput_Identify(TIM_HandleTypeDef *htim) // function to identify to which button is pressed
{
  if (htim->Channel == Red.input.Channel_IC)
  {
    ButtonInput_InterruptRoutine(&Red.input);
  }
  else if (htim->Channel == Orange.input.Channel_IC)
  {
    ButtonInput_InterruptRoutine(&Orange.input);
  }
  else if (htim->Channel == Green.input.Channel_IC)
  {
    ButtonInput_InterruptRoutine(&Green.input);
  }
}
void ButtonInput_InterruptRoutine(ButtonInput *button) // routine to be called after input capture callback (updates button structure)
{
  uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(button->Timer, button->Channel);

  /* Calculate difference */
  uint32_t Difference = 0;
  if (button->RisingCCR > CurrentCCR)
  {
    Difference = ((0xffff - button->RisingCCR) + CurrentCCR) * 1000 / button->refClock;
  }
  else
  {
    Difference = (CurrentCCR - button->RisingCCR) * 1000 / button->refClock;
  }

  if (Difference < BUTTON_INPUT_MINIMUM_PULSE_TIME)
  {
    button->RisingCCR = CurrentCCR;
  }

  button->State = HAL_GPIO_ReadPin(button->GPIOx, button->GPIO_Pin);

  if (button->State == (GPIO_PinState)SET) // Rising edge
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
    else if (Difference < BUTTON_INPUT_DOUBLE_CLICK_TIME)
    {
      /* single click */
      button->Type = CLICK_TYPE_SINGLE_CLICK;
      button->newType = true;
    }
  }
}
Button_ClickType ButtonInput_GetType(Button_ID id) // get click type if avaliable
{
  ButtonInput *inst;
  switch (id)
  {
  case RED_BUTTON_ID:
    inst = &Red.input;
    break;
  case ORANGE_BUTTON_ID:
    inst = &Orange.input;
    break;
  case GREEN_BUTTON_ID:
    inst = &Green.input;
    break;
  default:
    return 0;
    break;
  }
  if (inst->newType == true)
  {
    inst->newType = false;
    return (inst->Type);
  }
  return CLICK_TYPE_NONE;
}
void ButtonInput_Stop(ButtonInput *button) // stop Input Capture for selected button
{
  HAL_TIM_IC_Stop_IT(button->Timer, button->Channel);
}
void ButtonInput_Stop_Timer(ButtonInput *button) // stops timer base
{
  HAL_TIM_Base_Stop_IT(button->Timer);
}
/* BUTTON INPUT END */

/* Menu specific functions */
void ButtonLight_BreathAllOnce(void) // breath all button lights once
{
  ButtonLight_Set_Breathe(&Red.light, 100, 2);
  ButtonLight_Set_Breathe(&Orange.light, 100, 2);
  ButtonLight_Set_Breathe(&Green.light, 100, 2);
}
void ButtonLight_BlinkAllOnce(void) // blink all button lights once
{
  ButtonLight_Set_Blink(&Red.light, 100, 1);
  ButtonLight_Set_Blink(&Orange.light, 100, 1);
  ButtonLight_Set_Blink(&Green.light, 100, 1);
}
void ButtonLight_BreathOneOnce(Button_ID id) // breath selected button once
{
  switch (id)
  {
  case RED_BUTTON_ID:
    ButtonLight_Set_Breathe(&Red.light, 100, 1);
    break;
  case ORANGE_BUTTON_ID:
    ButtonLight_Set_Breathe(&Orange.light, 100, 1);
    break;
  case GREEN_BUTTON_ID:
    ButtonLight_Set_Breathe(&Green.light, 100, 1);
    break;

  default:
    break;
  }
}
void ButtonLight_BlinkOneOnce(Button_ID id) // blink selected button once
{
  switch (id)
  {
  case RED_BUTTON_ID:
    ButtonLight_Set_Blink(&Red.light, 100, 1);
    break;
  case ORANGE_BUTTON_ID:
    ButtonLight_Set_Blink(&Orange.light, 100, 1);
    break;
  case GREEN_BUTTON_ID:
    ButtonLight_Set_Blink(&Green.light, 100, 1);
    break;

  default:
    break;
  }
}
void ButtonLight_SetOne(Button_ID id, bool state) // set selected button
{
  uint8_t Mode;
  if (state != false)
  {
    Mode = BUTTON_MODE_ON;
  }
  else
  {
    Mode = BUTTON_MODE_RESTING;
  }

  // if animation is not active then display state
  switch (id)
  {
  case RED_BUTTON_ID:
    if (Red.light.Mode != BUTTON_MODE_ANIMATED)
    {
      ButtonLight_Set_Mode(&Red.light, Mode);
    }
    break;
  case ORANGE_BUTTON_ID:
    if (Orange.light.Mode != BUTTON_MODE_ANIMATED)
    {
      ButtonLight_Set_Mode(&Orange.light, Mode);
      break;
    }
  case GREEN_BUTTON_ID:
    if (Green.light.Mode != BUTTON_MODE_ANIMATED)
    {
      ButtonLight_Set_Mode(&Green.light, Mode);
      break;
    }

  default:
    break;
  }
}
void ButtonLight_Breath(Button_ID id, uint8_t Speed, uint16_t Count) // breath one button
{
  switch (id)
  {
  case RED_BUTTON_ID:
    ButtonLight_Set_Breathe(&Red.light, Speed, Count);
    break;
  case ORANGE_BUTTON_ID:
    ButtonLight_Set_Breathe(&Orange.light, Speed, Count);
    break;
  case GREEN_BUTTON_ID:
    ButtonLight_Set_Breathe(&Green.light, Speed, Count);
    break;

  default:
    break;
  }
}
void ButtonLight_Blink(Button_ID id, uint8_t Speed, uint16_t Count) // blink one button
{
  switch (id)
  {
  case RED_BUTTON_ID:
    ButtonLight_Set_Blink(&Red.light, Speed, Count);
    break;
  case ORANGE_BUTTON_ID:
    ButtonLight_Set_Blink(&Orange.light, Speed, Count);
    break;
  case GREEN_BUTTON_ID:
    ButtonLight_Set_Blink(&Green.light, Speed, Count);
    break;

  default:
    break;
  }
}