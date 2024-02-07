/**
 ******************************************************************************
 * @file    nECU_main.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_main.c file
 */
#ifndef _NECU_MAIN_H_
#define _NECU_MAIN_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
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