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
#define SPI_1_TIMEOUT 10 // timeout of communication in miliseconds

  /* Function Prototypes */
  void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi); // called when successfully recived data
  void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);  // called when error ocured

  void nECU_SPI_Rx_IT_Start(GPIO_struct *pCS_pin, nECU_SPI_ID ID, uint8_t *pData, uint16_t Size); // start communication with selected device
  void nECU_SPI_IT_Stop(nECU_SPI_ID ID);                                                          // end IT communication with selected device

  static bool nECU_SPI_getBusy(nECU_SPI_ID ID); // get state (if busy) of the SPI communication

  static nECU_SPI_ID nECU_SPI_Identify(SPI_HandleTypeDef *hspi); // returns ID of given input
  SPI_HandleTypeDef *nECU_SPI_getPointer(nECU_SPI_ID ID);        // returns pointer to

#ifdef __cplusplus
}
#endif

#endif /* _NECU_SPI_H_ */