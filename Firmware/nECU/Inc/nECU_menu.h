/**
 ******************************************************************************
 * @file    nECU_menu.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_menu.c file
 */
#ifndef nECU_menu_H_
#define nECU_menu_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "nECU_menu.h"
#include "nECU_types.h"
#include "nECU_button.h"

/* Definitions */
#define TUNE_NUMBER 3              // number of programmed tunes
#define LAUNCH_CONTROL_NUMBER 4    // number of avaliable launch control levels
#define FLASH_SAVE_DELAY_TIME 5000 // time to wait for before save happens in ms

  /* Button logic */
  bool Button_Menu_Init(void); // initialize button menu
  void Button_Menu(void);      // update function
  /* Interface functions */
  bool *Button_Menu_getPointer_Antilag(void);
  bool *Button_Menu_getPointer_TractionOFF(void);
  bool *Button_Menu_getPointer_ClearEngineCode(void);
  uint16_t *Button_Menu_getPointer_LunchControlLevel(void);
  uint16_t *Button_Menu_getPointer_TuneSelector(void);

  /* TachoValue interface functions */
  uint8_t *TachoValue_Get_OutputPointer(Tacho_ID ID); // get pointer to correct structure value
  bool *TachoValue_Get_ShowPointer(Tacho_ID ID);      // get pointer to correct structure value
  void TachoValue_Clear_ShowPending(Tacho_ID ID);     // clear pending flag for selected struct
  void TachoValue_Update_All(void);                   // update all TachoValue structures
  bool TachoValue_Init_All(void);                     // initialize tachometer value structures
  /* TachoValue internal functions */
  static bool TachoValue_Init_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier); // initialize single structure
  static void TachoValue_Update_Single(TachoValue *inst);                                           // update variables when needed

#ifdef __cplusplus
}
#endif

#endif /* __nECU_menu_H__ */