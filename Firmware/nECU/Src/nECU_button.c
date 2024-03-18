/**
 ******************************************************************************
 * @file    nECU_button.c
 * @brief   This file provides code for button inputs and their backlight.
 ******************************************************************************
 */

#include "nECU_button.h"

Button Red;
Button Orange;
Button Green;

static bool Red_Initialized = false, Red_Working = false;
static bool Orange_Initialized = false, Orange_Working = false;
static bool Green_Initialized = false, Green_Working = false;

/* All button functions */
void Button_Start(void)
{
  ButtonLight_Init(&Red.light, 1, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Red.input, 1, &BUTTON_INPUT_TIMER);
  Red_Initialized = true;
  Red_Working = true;

  ButtonLight_Init(&Orange.light, 2, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Orange.input, 2, &BUTTON_INPUT_TIMER);
  Orange_Initialized = true;
  Orange_Working = true;

  ButtonLight_Init(&Green.light, 3, &BUTTON_OUTPUT_TIMER);
  ButtonInput_Init(&Green.input, 3, &BUTTON_INPUT_TIMER);
  Green_Initialized = true;
  Green_Working = true;
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

  Red_Working = false;
  Orange_Working = false;
  Green_Working = false;
}

/* BUTTON LIGHT BEGIN */
void ButtonLight_Init(ButtonLight *Light, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonLight object with corresponding timer
{
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
    break;
  }
  nECU_tim_PWM_start(&Light->Timer);

  nECU_TickTrack_Init(&(Light->TimeTracker));

  Light->Mode = BUTTON_MODE_OFF; // Turn off
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
    if (Light->Breathing.count > 0)
    {
      Light->Breathing.state = ((uint16_t)(Light->Time * Light->Timer.period * BUTTON_LIGHT_BREATHING_SPEEDUP / Light->Breathing.speed) % 255) - 128;

      if (Light->Breathing.state != Light->Breathing.prevState)
      {
        if (Light->Breathing.state < 0 && Light->Breathing.prevState > 0)
        {
          Light->Breathing.count--;
        }
        Light->Breathing.prevState = Light->Breathing.state;
      }

      if (Light->Breathing.state < 0)
      {
        Light->Breathing.state = -Light->Breathing.state;
      }

      Light->Brightness = (((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * Light->Breathing.state) / 128) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->Blinking.count > 0)
    {
      Light->Blinking.state = (uint32_t)(Light->Time * Light->Timer.period / (Light->Blinking.speed * BUTTON_LIGHT_BLINKING_SLOWDOWN)) % 2;
      if (Light->Blinking.state != Light->Blinking.prevState) // Execute only on positive edge
      {
        Light->Blinking.prevState = Light->Blinking.state;
        if (Light->Blinking.state == 1)
        {
          Light->Blinking.count--;
        }
      }
      Light->Brightness = ((BUTTON_LIGHT_MAXIMUM_INFILL - BUTTON_LIGHT_MINIMUM_INFILL) * Light->Blinking.state) + BUTTON_LIGHT_MINIMUM_INFILL;
    }

    if (Light->Breathing.count + Light->Blinking.count == 0)
    {
      Light->Blinking.state = 0;
      Light->Blinking.prevState = 0;
      Light->Mode = BUTTON_MODE_GO_TO_REST;
    }
    break;
  case BUTTON_MODE_ON:
    Light->Brightness = BUTTON_LIGHT_MAXIMUM_INFILL;
    break;
  default:
    break;
  }

  Light->CCR = Light->Brightness * (Light->Timer.htim->Init.Period / 100);

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
void ButtonLight_Set_Blink(ButtonLight *Light, uint8_t Speed, uint16_t Count) // setup blinking animation
{
  Light->Blinking.speed = Speed;
  Light->Blinking.count = Count;
  Light->Mode = BUTTON_MODE_ANIMATED;
}
void ButtonLight_Set_Breathe(ButtonLight *Light, uint8_t Speed, uint16_t Count) // setup breathing animation
{
  Light->Breathing.speed = Speed;
  Light->Breathing.count = Count;
  Light->Mode = BUTTON_MODE_ANIMATED;
}
void ButtonLight_Set_Mode(ButtonLight *Light, ButtonLight_Mode Mode) // setup mode (animation type)
{
  Light->Mode = Mode;
  Light->ModePrev = BUTTON_MODE_OFF;
}
void ButtonLight_UpdateAll(void) // function to launch updates for all buttons
{
  ButtonLight_TimeTrack_All(); // update times
  if (Red_Working == true)
  {
    ButtonLight_Update(&Red.light);
  }
  if (Orange_Working == true)
  {
    ButtonLight_Update(&Orange.light);
  }
  if (Green_Working == true)
  {
    ButtonLight_Update(&Green.light);
  }
}
void ButtonLight_TimeTrack(ButtonLight *Light) // funtion called to update time passed
{
  nECU_TickTrack_Update(&(Light->TimeTracker));                                 // update tracker
  Light->Time += Light->TimeTracker.difference * Light->TimeTracker.convFactor; // add to time elapsed
}
void ButtonLight_TimeTrack_All(void) // function called to update time in all buttons
{
  if (Red_Working == true)
  {
    ButtonLight_TimeTrack(&(Red.light));
  }
  if (Orange_Working == true)
  {
    ButtonLight_TimeTrack(&(Orange.light));
  }
  if (Green_Working == true)
  {
    ButtonLight_TimeTrack(&(Green.light));
  }
}
void ButtonLight_Stop(ButtonLight *Light) // stops PWM for seected button
{
  nECU_tim_PWM_stop(&Light->Timer);
}
/* BUTTON LIGHT END */

/* BUTTON INPUT BEGIN */
void ButtonInput_Init(ButtonInput *button, uint8_t Channel, TIM_HandleTypeDef *htim) // function to initialize ButtonInput object with corresponding timer and GPIO
{
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
    break;
  }
  nECU_tim_IC_start(&button->Timer);
}
void ButtonInput_TimingEvent(TIM_HandleTypeDef *htim) // funtion called after input capture interrupt from timer
{
  ButtonInput_Identify(htim);
}
void ButtonInput_Identify(TIM_HandleTypeDef *htim) // function to identify to which button is pressed
{
  if (htim->Channel == Red.input.Channel_IC)
  {
    if (Red_Working == true)
    {
      ButtonInput_InterruptRoutine(&Red.input);
    }
  }
  else if (htim->Channel == Orange.input.Channel_IC)
  {
    if (Orange_Working == true)
    {
      ButtonInput_InterruptRoutine(&Orange.input);
    }
  }
  else if (htim->Channel == Green.input.Channel_IC)
  {
    if (Green_Working == true)
    {
      ButtonInput_InterruptRoutine(&Green.input);
    }
  }
}
void ButtonInput_InterruptRoutine(ButtonInput *button) // routine to be called after input capture callback (updates button structure)
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
  nECU_tim_IC_stop(&button->Timer);
}
/* BUTTON INPUT END */

/* Animations */
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

  // if animation is not active then display buttonPin.State
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