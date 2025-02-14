/**
 ******************************************************************************
 * @file    nECU_can.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_can.c file
 */
#ifndef _NECU_CAN_H_
#define _NECU_CAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_tim.h"
#include "tim.h"
#include "can.h"

/* Definitions */
#define CAN_RX_WHEELSPEED_ID 0x400 // CAN message ID for Wheel Speed

  /* Function Prototypes */
  // General functions
  bool nECU_CAN_Start(void);                                               // start periodic transmission of EGT and Speed sensor data
  void nECU_CAN_WriteToBuffer(nECU_CAN_TX_Frame_ID frameID, uint8_t size); // copy input data to corresponding frame buffer
  bool nECU_CAN_Stop(void);                                                // stop all CAN code, with timing

  // TX functions
  void nECU_CAN_TX_CheckTime(void); // checks if it is time to send packet
  static bool nECU_CAN_TX_Init(nECU_CAN_TX_Frame_ID currentID);
  static bool nECU_CAN_TX_TransmitFrame(nECU_CAN_TX_Frame_ID frameID); // send selected frame over CAN
  static uint32_t nECU_CAN_TX_FindMailbox(CAN_HandleTypeDef *hcan);    // will find empty mailbox

  // RX functions
  static bool nECU_CAN_RX_Init(nECU_CAN_RX_Frame_ID currentID);
  void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan); // interrupt callback when new Rx frame in FIFO0
  static nECU_CAN_RX_Frame_ID nECU_CAN_RX_Identify(CAN_RxHeaderTypeDef *pHeader);
  static void nECU_CAN_RX_Update(nECU_CAN_RX_Frame_ID currentID, uint8_t *buf); // update value
  int32_t nECU_CAN_RX_getValue(nECU_CAN_RX_Frame_ID currentID);                 // returns value of given frame

  // Diagnostic functions
  static uint8_t nECU_CAN_IsBusy(void); // Check if any messages are pending
  bool nECU_CAN_GetError(void);         // get error state pf can periperal buisy

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */