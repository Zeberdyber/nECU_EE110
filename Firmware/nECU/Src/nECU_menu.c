/**
 ******************************************************************************
 * @file    nECU_menu.c
 * @brief   This file provides code for operating button menu. It specifies
 *          input and display actions.
 ******************************************************************************
 */

#include "nECU_menu.h"

ButtonMenu Menu;

TachoValue Tacho1, Tacho2, Tacho3;
static bool Tacho_Initialized = false; // flag indicating structures initialization
static bool Menu_Initialized = false, Menu_Working = false;

/* Button logic */
bool Button_Menu_Init(void) // initialize button menu
{
  bool status = false;

  if (Menu_Initialized == false)
  {
    Menu.Antilag = false;
    Menu.ClearEngineCode = false;
    Menu.LunchControlLevel = 0;
    Menu.MenuLevel = 0;
    Menu.TractionOFF = false;
    Menu.TuneSelector = 0;
    status |= nECU_readUserSettings(&(Menu.Antilag), &(Menu.TractionOFF));
    Menu_Initialized = true;
  }
  if (Menu_Working == false && Menu_Initialized == true)
  {
    status |= Button_Start();

    /* First animation */
    ButtonLight_Breath(RED_BUTTON_ID, 100, 1);
    ButtonLight_Breath(ORANGE_BUTTON_ID, 100, 1);
    ButtonLight_Breath(GREEN_BUTTON_ID, 100, 1);

    Menu_Working = true;
  }

  return status;
}
void Button_Menu(void) // update function
{
  if (Menu_Working == false)
  {
    return;
  }
  ButtonLight_UpdateAll();

  Button_ClickType RedType = ButtonInput_GetType(RED_BUTTON_ID);
  Button_ClickType OrangeType = ButtonInput_GetType(ORANGE_BUTTON_ID);
  Button_ClickType GreenType = ButtonInput_GetType(GREEN_BUTTON_ID);

  if (OrangeType == CLICK_TYPE_HOLD && Menu.MenuLevel < 1) // move up the menu
  {
    Menu.MenuLevel++;
    ButtonLight_Breath(RED_BUTTON_ID, 100, 1);
    ButtonLight_Breath(ORANGE_BUTTON_ID, 100, 1);
    ButtonLight_Breath(GREEN_BUTTON_ID, 100, 1);
  }
  else if ((OrangeType == CLICK_TYPE_DOUBLE_CLICK) && (Menu.MenuLevel > 0)) // move down the menu
  {
    Menu.MenuLevel--;
    ButtonLight_Breath(RED_BUTTON_ID, 100, 1);
    ButtonLight_Breath(ORANGE_BUTTON_ID, 100, 1);
    ButtonLight_Breath(GREEN_BUTTON_ID, 100, 1);
  }

  switch (Menu.MenuLevel)
  {
  case 0:
    if (RedType == CLICK_TYPE_SINGLE_CLICK) // Traction control ON/OFF
    {
      ButtonLight_Blink(RED_BUTTON_ID, 50, 1);
      Menu.TractionOFF = !Menu.TractionOFF;
      nECU_Save_Delay_Start();
    }
    else if (RedType == CLICK_TYPE_HOLD) // Calibrate speed sensors
    {
      ButtonLight_Breath(RED_BUTTON_ID, 100, 10); // temporary line as no speed sensor is working properly
      // Speed_CalibrateStart();
    }
    if (OrangeType == CLICK_TYPE_SINGLE_CLICK) // Antilag ON/OFF
    {
      ButtonLight_Blink(ORANGE_BUTTON_ID, 50, 1);
      Menu.Antilag = !Menu.Antilag;
      nECU_Save_Delay_Start();
    }
    if (GreenType == CLICK_TYPE_SINGLE_CLICK) // Lunch control level 1 ON/OFF
    {
      ButtonLight_Blink(GREEN_BUTTON_ID, 50, 1);
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
      ButtonLight_Blink(RED_BUTTON_ID, 50, 1);
      Menu.ClearEngineCode = true;
    }
    if ((OrangeType == CLICK_TYPE_SINGLE_CLICK) && (Menu.TuneSelector < TUNE_NUMBER)) // cycle threw tunes
    {
      ButtonLight_Blink(ORANGE_BUTTON_ID, 50, 1);
      Menu.TuneSelector++;
      if (Menu.TuneSelector == TUNE_NUMBER)
      {
        Menu.TuneSelector = 0;
      }
    }
    if ((GreenType == CLICK_TYPE_SINGLE_CLICK) && (Menu.LunchControlLevel < (LAUNCH_CONTROL_NUMBER + 1))) // cycle threw lunch control levels
    {
      ButtonLight_Blink(GREEN_BUTTON_ID, 50, 1);
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
    nECU_saveUserSettings(&(Menu.Antilag), &(Menu.TractionOFF));
    ButtonLight_Breath(ORANGE_BUTTON_ID, 100, 1);
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

/* TachoValue interface functions */
uint8_t *TachoValue_Get_OutputPointer(Tacho_ID ID) // get pointer to correct structure value
{
  switch (ID)
  {
  case TACHO_SHOW_1:
    return &Tacho1.output_value;
    break;
  case TACHO_SHOW_2:
    return &Tacho2.output_value;
    break;
  case TACHO_SHOW_3:
    return &Tacho3.output_value;
    break;
  default:
    break;
  }
}
bool *TachoValue_Get_ShowPointer(Tacho_ID ID) // get pointer to correct structure value
{
  switch (ID)
  {
  case TACHO_SHOW_1:
    return &Tacho1.showPending;
    break;
  case TACHO_SHOW_2:
    return &Tacho2.showPending;
    break;
  case TACHO_SHOW_3:
    return &Tacho3.showPending;
    break;
  default:
    break;
  }
}
void TachoValue_Clear_ShowPending(Tacho_ID ID) // clear pending flag for selected struct
{
  switch (ID)
  {
  case TACHO_SHOW_1:
    Tacho1.showPending == false;
    break;
  case TACHO_SHOW_2:
    Tacho2.showPending == false;
    break;
  case TACHO_SHOW_3:
    Tacho3.showPending == false;
    break;
  default:
    break;
  }
}
void TachoValue_Update_All(void) // update all TachoValue structures
{
  if (Tacho_Initialized == false)
  {
    return;
  }
  TachoValue_Update_Single(&Tacho1);
  TachoValue_Update_Single(&Tacho2);
  TachoValue_Update_Single(&Tacho3);
}
bool TachoValue_Init_All(void) // initialize tachometer value structures
{
  bool status = false;
  status |= TachoValue_Init_Single(&Tacho1, Button_Menu_getPointer_TuneSelector(), 10);
  status |= TachoValue_Init_Single(&Tacho2, Button_Menu_getPointer_LunchControlLevel(), 10);
  status |= TachoValue_Init_Single(&Tacho3, &Menu.MenuLevel, 10);
  Tacho_Initialized = true;

  return status;
}
/* TachoValue internal functions */
static bool TachoValue_Init_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier) // initialize single structure
{
  bool status = false;

  inst->showPending = false;
  inst->prev_input = *pinput_value;
  inst->input_value = pinput_value;
  inst->output_multiplier = multiplier;

  return status;
}
static void TachoValue_Update_Single(TachoValue *inst) // update variables when needed
{
  if (*inst->input_value != inst->prev_input)
  {
    inst->prev_input = *inst->input_value;
    inst->output_value = *inst->input_value * inst->output_multiplier;
    inst->showPending = true;
  }
}
