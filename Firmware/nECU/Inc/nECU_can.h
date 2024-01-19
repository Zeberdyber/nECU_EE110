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
#define CAN_TX_FRAME0_ID 0x500     // CAN message ID for frame 0
#define CAN_TX_FRAME1_ID 0x501     // CAN message ID for frame 1
#define CAN_TX_FRAME2_ID 0x502     // CAN message ID for frame 2
#define CAN_RX_WHEELSPEED_ID 0x400 // CAN message ID for Wheel Speed
#define CAN_RX_COOLANT_ID 0x401    // CAN message ID for Coolant Temperature

  /* typedef */

  /* Function Prototypes */
  // General functions
  void nECU_CAN_Start(void);                                               // start periodic transmission of EGT and Speed sensor data
  void nECU_CAN_WriteToBuffer(uint8_t frameNumber, uint8_t *TxData_Frame); // copy input data to corresponding frame buffer
  void nECU_CAN_Stop(void);                                                // stop all CAN code, with timing
  // Communication functions
  void nECU_CAN_TimerEvent(TIM_HandleTypeDef *htim);   // funtion called after periodic interrupt from timing timers
  void nECU_CAN_InitFrame(uint8_t frameNumber);        // initialize header for selected frame
  uint8_t nECU_CAN_TransmitFrame(uint8_t frameNumber); // send selected frame over CAN
  uint8_t nECU_CAN_IsBusy(void);                       // Check if any messages are pending
  // Diagnostic functions
  uint8_t nECU_CAN_GetStatus(void); // get current status of nECU CAN
  bool nECU_CAN_GetState(void);     // get data if can periperal buisy
  bool nECU_CAN_GetError(void);     // get error state pf can periperal buisy
  // Recive functions
  void nECU_CAN_RX_InitFrame(void);                                // initialize reciving frames with corresponding filters
  void nECU_CAN_RX_Stop(void);                                     // Disables Recive communication
  void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan); // interrupt callback when new Rx frame in FIFO0
  uint8_t *nECU_CAN_getCoolantPointer(void);                       // get pointer to the recived data of coolant variable
  uint8_t *nECU_CAN_getWheelSetupPointer(void);                    // get pointer to the recived data of wheel setup variable
  uint8_t *nECU_CAN_getRPMPointer(void);                           // get pointer to the recived data of RPM variable
  void nECU_CAN_RX_DecodeFrame0(void);                             // decode data from recived frame

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */