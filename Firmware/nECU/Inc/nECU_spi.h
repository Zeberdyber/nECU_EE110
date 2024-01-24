/**
 ******************************************************************************
 * @file    nECU_spi.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_spi.c file
 */

#ifndef _NECU_SPI_H_
#define _NECU_SPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_EGT.h"
#include "spi.h"

/* Definitions */
#define SPI_PERIPHERAL_EGT hspi1 // peripheral to which egt ICs are connected
  /* typedef */

  /* Function Prototypes */
  void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi); // called when successfully recived data
  void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);  // called when error ocured
  bool nECU_SPI_getBusy(void);                          // get state (if busy) of the SPI communication
  bool nECU_SPI_getError(void);                         // get state (if not ready and not busy)

#ifdef __cplusplus
}
#endif

#endif /* _NECU_SPI_H_ */