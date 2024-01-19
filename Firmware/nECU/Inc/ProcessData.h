/**
 ******************************************************************************
 * @file    ProcessData.h
 * @brief   This file contains all the function prototypes for
 *          the ProcessData.c file
 */
#ifndef PROCESSDATA_H_
#define PROCESSDATA_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU.h"

  /* Definitions */

  /* Unions define */

  /* type definitions*/

  /* Function Prototypes */
  /* General code */
  void nECU_Start(void); // start executing program (mostly in background)
  void nECU_main(void);  // main rutine of the program
  void nECU_Stop(void);  // stop all peripherals (no interrupts will generate)

#ifdef __cplusplus
}
#endif

#endif /* _PROCESSDATA_H_ */