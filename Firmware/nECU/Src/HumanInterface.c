/**
 ******************************************************************************
 * @file    HumanInterface.c
 * @brief   This file provides code for Interfacing with human. It specifies
 *          input and display methods.
 ******************************************************************************
 */

#include "HumanInterface.h"

OnBoardLED LED_L, LED_R;
bool LED_Initialized = false; // flag to triger initialization

ButtonMenu Menu;

uint16_t BlankUint16_t = 0; // place holder

TachoValue Tacho1, Tacho2, Tacho3;
bool TachoInitialized = false; // flag indicating structures initialization

/* TachoValue interface functions */
uint8_t *TachoValue_Get_OutputPointer(uint8_t structNumber) // get pointer to correct structure value
{
  switch (structNumber)
  {
  case 1:
    return &Tacho1.output_value;
    break;
  case 2:
    return &Tacho2.output_value;
    break;
  case 3:
    return &Tacho3.output_value;
    break;
  default:
    break;
  }
}
bool *TachoValue_Get_ShowPointer(uint8_t structNumber) // get pointer to correct structure value
{
  switch (structNumber)
  {
  case 1:
    return &Tacho1.showPending;
    break;
  case 2:
    return &Tacho2.showPending;
    break;
  case 3:
    return &Tacho3.showPending;
    break;
  default:
    break;
  }
}
void TachoValue_Clear_ShowPending(uint8_t structNumber) // clear pending flag for selected struct
{
  switch (structNumber)
  {
  case 1:
    Tacho1.showPending == false;
    break;
  case 2:
    Tacho2.showPending == false;
    break;
  case 3:
    Tacho3.showPending == false;
    break;
  default:
    break;
  }
}
void TachoValue_Update_All(void) // update all TachoValue structures
{
  if (TachoInitialized == false)
  {
    TachoValue_Init_All();
  }
  TachoValue_Update_Single(&Tacho1);
  TachoValue_Update_Single(&Tacho2);
  TachoValue_Update_Single(&Tacho3);
}
void TachoValue_Init_All(void) // initialize tachometer value structures
{
  TachoValue_Init_Single(&Tacho1, Button_Menu_getPointer_TuneSelector(), 10);
  TachoValue_Init_Single(&Tacho2, Button_Menu_getPointer_LunchControlLevel(), 10);
  TachoValue_Init_Single(&Tacho3, &Menu.MenuLevel, 10);
  TachoInitialized = true;
}
/* TachoValue internal functions */
void TachoValue_Init_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier) // initialize single structure
{
  inst->showPending = false;
  inst->prev_input = *pinput_value;
  inst->input_value = pinput_value;
  inst->output_multiplier = multiplier;
}
void TachoValue_Update_Single(TachoValue *inst) // update variables when needed
{
  if (*inst->input_value != inst->prev_input)
  {
    inst->prev_input = *inst->input_value;
    inst->output_value = *inst->input_value * inst->output_multiplier;
    inst->showPending = true;
  }
}

/* Button logic */
void Button_Menu_Init(void) // initialize button menu structure
{
  Menu.Antilag = false;
  Menu.ClearEngineCode = false;
  Menu.LunchControlLevel = 0;
  Menu.MenuLevel = 0;
  Menu.TractionOFF = false;
  Menu.TuneSelector = 0;
  Menu.initialized = true;
  ButtonLight_BreathAllOnce();
  nECU_readUserSettings(&(Menu.Antilag), &(Menu.TractionOFF));
}
void Button_Menu(void)
{
  Button_ClickType RedType = ButtonInput_GetType(RED_BUTTON_ID);
  Button_ClickType OrangeType = ButtonInput_GetType(ORANGE_BUTTON_ID);
  Button_ClickType GreenType = ButtonInput_GetType(GREEN_BUTTON_ID);

  if (OrangeType == CLICK_TYPE_HOLD && Menu.MenuLevel < 1) // move up the menu
  {
    Menu.MenuLevel++;
    ButtonLight_BreathAllOnce();
  }
  else if ((OrangeType == CLICK_TYPE_DOUBLE_CLICK) && (Menu.MenuLevel > 0)) // move down the menu
  {
    Menu.MenuLevel--;
    ButtonLight_BreathAllOnce();
  }

  switch (Menu.MenuLevel)
  {
  case 0:
    if (RedType == CLICK_TYPE_SINGLE_CLICK) // Traction control ON/OFF
    {
      ButtonLight_BlinkOneOnce(RED_BUTTON_ID);
      Menu.TractionOFF = !Menu.TractionOFF;
      nECU_Save_Delay_Start();
    }
    else if (RedType == CLICK_TYPE_HOLD) // Calibrate speed sensors
    {
      ButtonLight_Breath(RED_BUTTON_ID, 100, 10);
      // Speed_CalibrateStart();
    }
    if (OrangeType == CLICK_TYPE_SINGLE_CLICK) // Antilag ON/OFF
    {
      ButtonLight_BlinkOneOnce(ORANGE_BUTTON_ID);
      Menu.Antilag = !Menu.Antilag;
      nECU_Save_Delay_Start();
    }
    if (GreenType == CLICK_TYPE_SINGLE_CLICK) // Lunch control level 1 ON/OFF
    {
      ButtonLight_BlinkOneOnce(GREEN_BUTTON_ID);
      Menu.LunchControlLevel = !Menu.LunchControlLevel;
    }

    // display state (if no animation)
    ButtonLight_SetOne(RED_BUTTON_ID, Menu.TractionOFF);
    ButtonLight_SetOne(ORANGE_BUTTON_ID, Menu.Antilag);
    ButtonLight_SetOne(GREEN_BUTTON_ID, Menu.LunchControlLevel);
    break;
  case 1:
    if (RedType == CLICK_TYPE_SINGLE_CLICK) // Clear codes
    {
      ButtonLight_BlinkOneOnce(RED_BUTTON_ID);
      Menu.ClearEngineCode = true;
    }
    if ((OrangeType == CLICK_TYPE_SINGLE_CLICK) && (Menu.TuneSelector < TUNE_NUMBER)) // cycle threw tunes
    {
      ButtonLight_BlinkOneOnce(ORANGE_BUTTON_ID);
      Menu.TuneSelector++;
      if (Menu.TuneSelector == TUNE_NUMBER)
      {
        Menu.TuneSelector = 0;
      }
    }
    if ((GreenType == CLICK_TYPE_SINGLE_CLICK) && (Menu.LunchControlLevel < (LAUNCH_CONTROL_NUMBER + 1))) // cycle threw lunch control levels
    {
      ButtonLight_BlinkOneOnce(GREEN_BUTTON_ID);
      Menu.LunchControlLevel++;
      if (Menu.LunchControlLevel == (LAUNCH_CONTROL_NUMBER + 1)) // +1 for 0 state
      {
        Menu.LunchControlLevel = 0;
      }
    }

    // display state (if no animation)
    ButtonLight_SetOne(RED_BUTTON_ID, Menu.ClearEngineCode);
    ButtonLight_SetOne(ORANGE_BUTTON_ID, Menu.TuneSelector);
    ButtonLight_SetOne(GREEN_BUTTON_ID, Menu.LunchControlLevel);
    break;
  default:
    Menu.MenuLevel = 0;
    break;
  }
  bool *flash_save_due = nECU_Save_Delay_DoneFlag();
  if (*flash_save_due == true)
  {
    *flash_save_due = false;
    nECU_saveUserSettings(&(Menu.TractionOFF), &(Menu.TractionOFF));
    ButtonLight_Breath(ORANGE_BUTTON_ID, 100, 5);
  }
}

/* Interface functions */
bool *Button_Menu_getPointer_Antilag(void)
{
  return &(Menu.Antilag);
}
bool *Button_Menu_getPointer_TractionOFF(void)
{
  return &(Menu.TractionOFF);
}
bool *Button_Menu_getPointer_ClearEngineCode(void)
{
  return &(Menu.ClearEngineCode);
}
uint16_t *Button_Menu_getPointer_LunchControlLevel(void)
{
  return &(Menu.LunchControlLevel);
}
uint16_t *Button_Menu_getPointer_TuneSelector(void)
{
  return &(Menu.TuneSelector);
}

/* On board LEDs */
void OnBoard_LED_Init(void) // initialize structures for on board LEDs
{
  /* Left LED */
  LED_L.GPIO_Pin = LED1_Pin;
  LED_L.GPIOx = LED1_GPIO_Port;
  LED_L.LastTick = HAL_GetTick();

  /* Right LED */
  LED_R.GPIO_Pin = LED2_Pin;
  LED_R.GPIOx = LED2_GPIO_Port;
  LED_R.LastTick = HAL_GetTick();

  LED_Initialized = true;
}
void OnBoard_LED_UpdateSingle(OnBoardLED *inst) // function to perform logic behind blinking times and update to GPIO
{
  if (inst->blinking == true)
  {
    if ((HAL_GetTick() - inst->LastTick) > ((float)(ONBOARD_LED_MS_PER_BLINK / 2) * HAL_GetTickFreq()))
    {
      inst->LastTick = HAL_GetTick();
      inst->BlinkState = !inst->BlinkState;
    }
    inst->State = inst->BlinkState;
  }
  else
  {
    inst->LastTick = HAL_GetTick();
  }
  HAL_GPIO_WritePin(inst->GPIOx, inst->GPIO_Pin, inst->State);
}
void OnBoard_LED_Update(void) // update on board LEDs states
{
  if (LED_Initialized == false)
  {
    OnBoard_LED_Init();
  }

  LED_L.State = nECU_CAN_GetError();
  LED_L.blinking = nECU_CAN_GetState();
  OnBoard_LED_UpdateSingle(&LED_L);

  LED_R.State = nECU_SPI_getError();
  LED_R.blinking = nECU_SPI_getBusy();
  OnBoard_LED_UpdateSingle(&LED_R);
}
