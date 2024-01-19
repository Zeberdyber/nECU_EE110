/**
 ******************************************************************************
 * @file    HumanInterface.h
 * @brief   This file contains all the function prototypes for
 *          the HumanInterface.c file
 */
#ifndef HumanInterface_H_
#define HumanInterface_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "gpio.h"
#include "nECU_button.h"
#include "nECU_can.h"
#include "nECU_spi.h"

/* Definitions */
#define TUNE_NUMBER 3                // number of programmed tunes
#define LAUNCH_CONTROL_NUMBER 4      // number of avaliable launch control levels
#define ONBOARD_LED_MS_PER_BLINK 200 // number of miliseconds for full blink

  /* typedef */
  typedef struct
  {
    GPIO_TypeDef *GPIOx;      // GPIO group of the LED
    uint16_t GPIO_Pin;        // pin of the LED
    GPIO_PinState State;      // actual state of the output
    GPIO_PinState BlinkState; // state of the output (for blinking)
    bool blinking;            // flag when blinking
    uint32_t LastTick;        // tick at which last blink update performed
  } OnBoardLED;
  typedef struct
  {
    bool showPending;
    uint16_t *input_value;
    uint16_t prev_input;
    uint8_t output_value;
    uint8_t output_multiplier;
  } TachoValue;
  typedef struct
  {
    // output variables
    bool Antilag, TractionOFF, ClearEngineCode;
    uint16_t LunchControlLevel, TuneSelector;
    // internal variables
    uint16_t MenuLevel;
    bool initialized;
  } ButtonMenu;

  /* Function Prototypes */
  void Button_Menu_Init(void); // initialize button menu structure
  void Button_Menu(void);
  bool *Button_Menu_getPointer_Antilag(void);
  bool *Button_Menu_getPointer_TractionOFF(void);
  bool *Button_Menu_getPointer_ClearEngineCode(void);
  uint16_t *Button_Menu_getPointer_LunchControlLevel(void);
  uint16_t *Button_Menu_getPointer_TuneSelector(void);
  /* Show value on tachometer */
  uint8_t *TachoValue_Get_OutputPointer(uint8_t structNumber);                               // get pointer to correct structure value
  bool *TachoValue_Get_ShowPointer(uint8_t structNumber);                                    // get pointer to correct structure value
  void TachoValue_Clear_ShowPending(uint8_t structNumber);                                   // clear pending flag for selected struct
  void TachoValue_Update_All(void);                                                          // update all TachoValue structures
  void TachoValue_Init_All(void);                                                            // initialize tachometer value structures
  void TachoValue_Init_Single(TachoValue *inst, uint16_t *pinput_value, uint8_t multiplier); // initialize single structure
  void TachoValue_Update_Single(TachoValue *inst);                                           // update variables when needed

  /* On board LED */
  void OnBoard_LED_Init(void);                     // initialize structures for on board LEDs
  void OnBoard_LED_UpdateSingle(OnBoardLED *inst); // function to perform logic behind blinking times and update to GPIO
  void OnBoard_LED_Update(void);                   // update on board LEDs states

#ifdef __cplusplus
}
#endif

#endif /* __HumanInterface_H__ */