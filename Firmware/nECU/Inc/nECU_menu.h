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

  /* Menu */
  bool nECU_Menu_Start(void);
  bool nECU_Menu_Stop(void);
  void nECU_Menu_Routine(void);

  bool *nECU_Menu_Antilag_getPointer(void);
  bool *nECU_Menu_TractionOFF_getPointer(void);
  bool *nECU_Menu_ClearCode_getPointer(void);
  uint16_t *nECU_Menu_LunchLvl_getPointer(void);
  uint16_t *nECU_Menu_TuneSel_getPointer(void);

  /* Tacho */
  void nECU_Tacho_Routine(void);
  bool nECU_Tacho_Stop(void);
  bool nECU_Tacho_Start(void);

  uint8_t *nECU_Tacho_getPointer(Tacho_ID ID);   // get pointer to correct structure value
  bool *nECU_Tacho_Show_getPointer(Tacho_ID ID); // get pointer to correct structure value
  void nECU_Tacho_Clear_getPointer(Tacho_ID ID); // clear pending flag for selected struct

  /* TachoValue internal functions */
  static bool nECU_Tacho_Start_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier); // initialize single structure
  static void nECU_Tacho_Routine_Single(TachoValue *inst);                                           // update variables when needed

#ifdef __cplusplus
}
#endif

#endif /* __nECU_menu_H__ */