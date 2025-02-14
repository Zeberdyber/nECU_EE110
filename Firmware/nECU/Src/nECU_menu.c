/**
 ******************************************************************************
 * @file    nECU_menu.c
 * @brief   This file provides code for operating button menu. It specifies
 *          input and display actions.
 ******************************************************************************
 */

#include "nECU_menu.h"

static ButtonMenu Menu = {0};
static TachoValue Tacho[TACHO_ID_MAX];

/* Menu */
bool nECU_Menu_Start(void)
{
  bool status = false;

  if (!nECU_FlowControl_Initialize_Check(D_Menu))
  {
    Menu.Antilag = false;
    Menu.ClearCode = false;
    Menu.LunchLvl = 0;
    Menu.MenuLvl = 0;
    Menu.TractionOFF = false;
    Menu.TuneSel = 0;

    status |= nECU_Delay_Set(&(Menu.save_delay), FLASH_SAVE_DELAY_TIME);

    if (!status)
      status |= !nECU_FlowControl_Initialize_Do(D_Menu);
  }
  if (!nECU_FlowControl_Working_Check(D_Menu) && status == false)
  {
    status |= nECU_FLASH_Start();
    status |= nECU_Button_Start(BUTTON_ID_RED);
    status |= nECU_Button_Start(BUTTON_ID_ORANGE);
    status |= nECU_Button_Start(BUTTON_ID_GREEN);

    if (!status)
      status |= nECU_Flash_UserSettings_read(&(Menu.Antilag), &(Menu.TractionOFF));

    /* First animation */
    // nECU_Button_Light_Breath(BUTTON_ID_RED, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_ORANGE, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_GREEN, 100, 1);

    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_Menu);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_Menu);

  return status;
}
bool nECU_Menu_Stop(void)
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_Menu) && status == false)
  {
    status |= nECU_Delay_Stop(&(Menu.save_delay));
    status |= nECU_Flash_UserSettings_save(&(Menu.Antilag), &(Menu.TractionOFF));

    status |= nECU_Button_Stop(BUTTON_ID_RED);
    status |= nECU_Button_Stop(BUTTON_ID_ORANGE);
    status |= nECU_Button_Stop(BUTTON_ID_GREEN);

    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_Menu);

    status |= nECU_FLASH_Stop();
  }
  if (status)
    nECU_FlowControl_Error_Do(D_Menu);

  return status;
}
void nECU_Menu_Routine(void)
{
  if (!nECU_FlowControl_Working_Check(D_Menu)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_Menu);
    return; // Break
  }

  // nECU_Button_Light_Routine_All();
  nECU_Delay_Update(&(Menu.save_delay));

  // Button_ClickType RedType = nECU_Button_Input_GetType(BUTTON_ID_RED);
  // Button_ClickType OrangeType = nECU_Button_Input_GetType(BUTTON_ID_ORANGE);
  // Button_ClickType GreenType = nECU_Button_Input_GetType(BUTTON_ID_GREEN);
  Button_ClickType RedType, OrangeType, GreenType;
  if (OrangeType == CLICK_TYPE_HOLD && Menu.MenuLvl < 1) // move up the menu
  {
    Menu.MenuLvl++;
    // nECU_Button_Light_Breath(BUTTON_ID_RED, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_ORANGE, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_GREEN, 100, 1);
  }
  else if ((OrangeType == CLICK_TYPE_DOUBLE_CLICK) && (Menu.MenuLvl > 0)) // move down the menu
  {
    Menu.MenuLvl--;
    // nECU_Button_Light_Breath(BUTTON_ID_RED, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_ORANGE, 100, 1);
    // nECU_Button_Light_Breath(BUTTON_ID_GREEN, 100, 1);
  }

  switch (Menu.MenuLvl)
  {
  case 0:
    if (RedType == CLICK_TYPE_SINGLE_CLICK) // Traction control ON/OFF
    {
      // nECU_Button_Light_Blink(BUTTON_ID_RED, 50, 1);
      Menu.TractionOFF = !Menu.TractionOFF;
      nECU_Delay_Start(&(Menu.save_delay));
    }
    else if (RedType == CLICK_TYPE_HOLD) // Calibrate speed sensors
    {
      // nECU_Button_Light_Breath(BUTTON_ID_RED, 100, 10); // temporary line as no speed sensor is working properly
      // Speed_CalibrateStart();
    }
    if (OrangeType == CLICK_TYPE_SINGLE_CLICK) // Antilag ON/OFF
    {
      // nECU_Button_Light_Blink(BUTTON_ID_ORANGE, 50, 1);
      Menu.Antilag = !Menu.Antilag;
      nECU_Delay_Start(&(Menu.save_delay));
    }
    if (GreenType == CLICK_TYPE_SINGLE_CLICK) // Lunch control level 1 ON/OFF
    {
      // nECU_Button_Light_Blink(BUTTON_ID_GREEN, 50, 1);
      Menu.LunchLvl = !Menu.LunchLvl;
    }

    // display state (if no animation)
    // nECU_Button_Light_SetOne(BUTTON_ID_RED, Menu.TractionOFF);
    // nECU_Button_Light_SetOne(BUTTON_ID_ORANGE, Menu.Antilag);
    // nECU_Button_Light_SetOne(BUTTON_ID_GREEN, Menu.LunchLvl);
    break;
  case 1:
    if (RedType == CLICK_TYPE_SINGLE_CLICK) // Clear codes
    {
      // nECU_Button_Light_Blink(BUTTON_ID_RED, 50, 1);
      Menu.ClearCode = true;
    }
    if ((OrangeType == CLICK_TYPE_SINGLE_CLICK) && (Menu.TuneSel < TUNE_NUMBER)) // cycle threw tunes
    {
      // nECU_Button_Light_Blink(BUTTON_ID_ORANGE, 50, 1);
      Menu.TuneSel++;
      if (Menu.TuneSel == TUNE_NUMBER)
      {
        Menu.TuneSel = 0;
      }
    }
    if ((GreenType == CLICK_TYPE_SINGLE_CLICK) && (Menu.LunchLvl < (LAUNCH_CONTROL_NUMBER + 1))) // cycle threw lunch control levels
    {
      // nECU_Button_Light_Blink(BUTTON_ID_GREEN, 50, 1);
      Menu.LunchLvl++;
      if (Menu.LunchLvl == (LAUNCH_CONTROL_NUMBER + 1)) // +1 for 0 state
      {
        Menu.LunchLvl = 0;
      }
    }

    // display state (if no animation)
    // nECU_Button_Light_SetOne(BUTTON_ID_RED, Menu.ClearCode);
    // nECU_Button_Light_SetOne(BUTTON_ID_ORANGE, Menu.TuneSel);
    // nECU_Button_Light_SetOne(BUTTON_ID_GREEN, Menu.LunchLvl);
    break;
  default:
    Menu.MenuLvl = 0;
    nECU_FlowControl_Error_Do(D_Menu);
    break;
  }
  if (*nECU_Delay_DoneFlag(&(Menu.save_delay)) == true) // Perform save
  {
    nECU_Delay_Stop(&(Menu.save_delay));
    nECU_Flash_UserSettings_save(&(Menu.Antilag), &(Menu.TractionOFF));
    // nECU_Button_Light_Breath(BUTTON_ID_ORANGE, 100, 1);
  }

  nECU_Debug_ProgramBlockData_Update(D_Menu);
}

bool *nECU_Menu_Antilag_getPointer(void)
{
  return &(Menu.Antilag);
}
bool *nECU_Menu_TractionOFF_getPointer(void)
{
  return &(Menu.TractionOFF);
}
bool *nECU_Menu_ClearCode_getPointer(void)
{
  return &(Menu.ClearCode);
}
uint16_t *nECU_Menu_LunchLvl_getPointer(void)
{
  return &(Menu.LunchLvl);
}
uint16_t *nECU_Menu_TuneSel_getPointer(void)
{
  return &(Menu.TuneSel);
}

/* Tacho */
bool nECU_Tacho_Start(void)
{
  bool status = false;

  if (!nECU_FlowControl_Initialize_Check(D_Tacho))
  {
    if (!status)
      status |= !nECU_FlowControl_Initialize_Do(D_Tacho);
  }
  if (!nECU_FlowControl_Working_Check(D_Tacho) && status == false)
  {
    status |= nECU_Tacho_Start_Single(&Tacho[TACHO_ID_TuneSel], nECU_Menu_TuneSel_getPointer(), 10);
    status |= nECU_Tacho_Start_Single(&Tacho[TACHO_ID_LunchLvl], nECU_Menu_LunchLvl_getPointer(), 10);
    status |= nECU_Tacho_Start_Single(&Tacho[TACHO_ID_MenuLvl], &Menu.MenuLvl, 10);

    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_Tacho);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_Tacho);

  return status;
}
bool nECU_Tacho_Stop(void)
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_Tacho) && status == false)
  {
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_Tacho);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_Tacho);

  return status;
}
void nECU_Tacho_Routine(void)
{
  if (!nECU_FlowControl_Working_Check(D_Tacho)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_Tacho);
    return; // Break
  }
  nECU_Tacho_Routine_Single(&Tacho[TACHO_ID_TuneSel]);
  nECU_Tacho_Routine_Single(&Tacho[TACHO_ID_LunchLvl]);
  nECU_Tacho_Routine_Single(&Tacho[TACHO_ID_MenuLvl]);

  nECU_Debug_ProgramBlockData_Update(D_Tacho);
}

uint8_t *nECU_Tacho_getPointer(Tacho_ID ID) // get pointer to correct structure value
{
  if (ID >= TACHO_ID_MAX) // Break if invalid ID
    return NULL;

  return &Tacho[ID].output_value;
}
bool *nECU_Tacho_Show_getPointer(Tacho_ID ID) // get pointer to correct structure value
{
  if (ID >= TACHO_ID_MAX) // Break if invalid ID
    return NULL;

  return &Tacho[ID].showPending;
}
void nECU_Tacho_Clear_getPointer(Tacho_ID ID) // clear pending flag for selected struct
{
  if (ID >= TACHO_ID_MAX) // Break if invalid ID
    return;

  if (!nECU_FlowControl_Working_Check(D_Tacho)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_Tacho);
    return; // Break
  }
  Tacho[ID].showPending = false;
}

/* TachoValue internal functions */
static bool nECU_Tacho_Start_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier) // initialize single structure
{
  bool status = false;

  inst->showPending = false;
  inst->prev_input = *pinput_value;
  inst->input_value = pinput_value;
  inst->output_multiplier = multiplier;

  return status;
}
static void nECU_Tacho_Routine_Single(TachoValue *inst) // update variables when needed
{
  if (*inst->input_value != inst->prev_input)
  {
    inst->prev_input = *inst->input_value;
    inst->output_value = *inst->input_value * inst->output_multiplier;
    inst->showPending = true;
  }
}
