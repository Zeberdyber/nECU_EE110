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
  void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);                                                                        // called when successfully recived data
  void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);                                                                         // called when error ocured
  bool nECU_SPI_getBusy(SPI_HandleTypeDef *hspi);                                                                              // get state (if busy) of the SPI communication
  bool nECU_SPI_getError(SPI_HandleTypeDef *hspi);                                                                             // get error type (if not ready and not busy)
  void nECU_SPI_Rx_DMA_Start(GPIO_TypeDef *GPIOx, uint16_t *GPIO_Pin, SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size); // start communication with selected device
  void nECU_SPI_Rx_DMA_Stop(GPIO_TypeDef *GPIOx, uint16_t *GPIO_Pin, SPI_HandleTypeDef *hspi);                                 // end communication with selected device

#ifdef __cplusplus
}
#endif

#endif /* _NECU_SPI_H_ */