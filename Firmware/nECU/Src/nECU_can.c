/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          timing.
 ******************************************************************************
 */

#include "nECU_can.h"
#include "nECU_tests.h"

/* CAN RX */
static bool Rx_Data_Frame0_Pending = false;
static CAN_FilterTypeDef WheelSetupFilter = {0};
static uint8_t RxData_Frame0[8] = {0};
static CAN_RxHeaderTypeDef CanRxFrame0 = {0};

static nECU_CAN_Tx_Data frame_List[nECU_Frame_ID_MAX] = {0};
static uint32_t const delay_List[nECU_Frame_ID_MAX] = {
    [nECU_Frame_Speed_ID] = 100,
    [nECU_Frame_EGT_ID] = 100,
    [nECU_Frame_Stock_ID] = 10,
};
static uint32_t const Msg_ID_List[nECU_Frame_ID_MAX] = {
    [nECU_Frame_Speed_ID] = 0x500,
    [nECU_Frame_EGT_ID] = 0x501,
    [nECU_Frame_Stock_ID] = 0x502,
};

// General functions
bool nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  bool status_TX = false, status_RX = false;

  /* TX */
  if (!nECU_FlowControl_Initialize_Check(D_CAN_TX) && status_TX == false)
  {
    for (nECU_CAN_Frame_ID current_ID = 0; current_ID < nECU_Frame_ID_MAX; current_ID++)
    {
      // delay
      status_TX |= nECU_Delay_Set(&(frame_List[current_ID].frame_delay), delay_List[current_ID]);
      status_TX |= nECU_Delay_Start(&(frame_List[current_ID].frame_delay));
      // default values
      frame_List[current_ID].can_data.Header.StdId = Msg_ID_List[current_ID];
      frame_List[current_ID].can_data.Header.IDE = CAN_ID_STD;
      frame_List[current_ID].can_data.Header.RTR = CAN_RTR_DATA;
      memset(frame_List[current_ID].can_data.Buffer, 0, sizeof(frame_List[current_ID].can_data.Buffer)); // clear buffer
      // data pointer
      if (nECU_Frame_getPointer(current_ID))
        frame_List[current_ID].buf.Buffer = nECU_Frame_getPointer(current_ID);
      else
        status_TX |= true;
    }
    if (!status_TX)
      status_TX |= !nECU_FlowControl_Initialize_Do(D_CAN_TX);
  }
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) && status_TX == false)
  {
    if (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_READY)
      status_TX |= (HAL_CAN_Start(&hcan1) != HAL_OK);

    if (!status_TX)
    {
      status_TX |= !nECU_FlowControl_Working_Do(D_CAN_TX);
    }
  }
  if (status_TX)
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
  }

  /* RX */
  if (!nECU_FlowControl_Initialize_Check(D_CAN_RX) && status_RX == false)
  {
    status_RX |= nECU_CAN_RX_InitFrame();
    if (!status_RX)
    {
      status_RX |= !nECU_FlowControl_Initialize_Do(D_CAN_RX);
    }
  }
  if (!nECU_FlowControl_Working_Check(D_CAN_RX) && status_RX == false)
  {
    if (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_READY)
      status_RX |= (HAL_CAN_Start(&hcan1) != HAL_OK);

    if (!status_RX)
    {
      status_RX |= !nECU_FlowControl_Working_Do(D_CAN_RX);
    }
  }
  if (status_RX)
  {
    nECU_FlowControl_Error_Do(D_CAN_RX);
  }

  return status_TX | status_RX;
}
void nECU_CAN_WriteToBuffer(nECU_CAN_Frame_ID frameID, uint8_t size) // copy input data to corresponding frame buffer
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) || (frameID > nECU_Frame_ID_MAX) || size == 0)
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    return;
  }

  if (size > 8) // cap max length
    size = 8;

  frame_List[frameID].can_data.Header.DLC = size;
  memcpy(frame_List[frameID].can_data.Buffer,
         frame_List[frameID].buf.Buffer,
         frame_List[frameID].can_data.Header.DLC); // copy data to can buffer
}
bool nECU_CAN_Stop(void) // stop all CAN code, with timing
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_CAN_TX) && status == false)
  {
    for (nECU_CAN_Frame_ID currenID = 0; currenID < nECU_Frame_ID_MAX; currenID++)
      status |= nECU_Delay_Stop(&(frame_List[currenID].frame_delay));

    status |= nECU_CAN_RX_Stop();
    status |= (HAL_OK != HAL_CAN_Stop(&hcan1));
    if (!status)
      status |= nECU_FlowControl_Stop_Do(D_CAN_TX);
  }
  return status;
}
void nECU_CAN_CheckTime(void) // checks if it is time to send packet
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    return;
  }
  for (nECU_CAN_Frame_ID currentID = 0; currentID < nECU_Frame_ID_MAX; currentID++)
  {
    if (nECU_FlowControl_Working_Check(D_Frame_Speed_ID + currentID)) // check if frame working
    {
      nECU_Delay_Update(&(frame_List[currentID].frame_delay)); // update time
      if (frame_List[currentID].frame_delay.done)              // check if time is above threshold
      {
        nECU_Delay_Start(&(frame_List[currentID].frame_delay)); // restart delay

        if (nECU_CAN_TX_TransmitFrame(currentID))
          nECU_FlowControl_Error_Do(D_Frame_Speed_ID) + currentID;
        else
          nECU_Frame_TX_done(currentID);
      }
    }
  }

  nECU_Debug_ProgramBlockData_Update(D_CAN_TX);
}

// Communication functions
static bool nECU_CAN_TX_TransmitFrame(nECU_CAN_Frame_ID frameID) // send selected frame over CAN
{
  bool status = false;
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) || (frameID > nECU_Frame_ID_MAX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    status |= true;
    return status;
  }

  uint32_t mail = nECU_CAN_FindMailbox(&hcan1);
  status |= (HAL_CAN_AddTxMessage(&hcan1, &(frame_List[frameID].can_data.Header), frame_List[frameID].can_data.Buffer, &(mail)) != HAL_OK); // Transmit the data

  return status;
}

static uint32_t nECU_CAN_FindMailbox(CAN_HandleTypeDef *hcan) // will find empty mailbox
{
  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan))                                                     // check if any mailbox is empty
    for (uint32_t Mailbox = CAN_TX_MAILBOX0; Mailbox <= CAN_TX_MAILBOX2; Mailbox = Mailbox << 1) // go threw all mailboxes
      if (!HAL_CAN_IsTxMessagePending(&hcan1, Mailbox))                                          // check mailbox if empty
        return Mailbox;

  return 8; // return non-valid mailbox
}

// Diagnostic functions
static uint8_t nECU_CAN_IsBusy(void) // Check if any messages are pending
{
  return 3 - HAL_CAN_GetTxMailboxesFreeLevel(&hcan1);
}
bool nECU_CAN_GetError(void) // get error state pf can periperal buisy
{
  HAL_CAN_StateTypeDef CurrentState = HAL_CAN_GetState(&hcan1);
  if (CurrentState >= HAL_CAN_STATE_SLEEP_PENDING || CurrentState == HAL_CAN_STATE_RESET)
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    nECU_FlowControl_Error_Do(D_CAN_RX);
    return true;
  }
  return false;
}

// Rx functions
static bool nECU_CAN_RX_InitFrame(void) // initialize reciving frames with corresponding filters
{
  bool status = false;

  // Wheel setup and coolant
  WheelSetupFilter.FilterBank = 0;
  WheelSetupFilter.FilterMode = CAN_FILTERMODE_IDMASK;
  WheelSetupFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
  WheelSetupFilter.FilterIdHigh = CAN_RX_WHEELSPEED_ID << 5;
  WheelSetupFilter.FilterIdLow = 0;
  WheelSetupFilter.FilterMaskIdHigh = CAN_RX_WHEELSPEED_ID << 5;
  WheelSetupFilter.FilterMaskIdLow = 0;
  WheelSetupFilter.FilterScale = CAN_FILTERSCALE_32BIT;
  WheelSetupFilter.FilterActivation = ENABLE;
  WheelSetupFilter.SlaveStartFilterBank = 14;

  status |= (HAL_CAN_ConfigFilter(&hcan1, &WheelSetupFilter) != HAL_OK);                   // Initialize CAN Filter
  status |= (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK); // Initialize CAN Bus Rx Interrupt

  return status;
}
bool nECU_CAN_RX_Stop(void) // Disables Recive communication
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_CAN_RX) && status == false)
  {
    status |= (HAL_OK != HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING)); // Stop waiting for RX
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_CAN_RX);
  }
  return status;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // interrupt callback when new Rx frame in FIFO0
{
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CanRxFrame0, RxData_Frame0); // Receive CAN bus message to canRX buffer
  Rx_Data_Frame0_Pending = true;
  nECU_Debug_ProgramBlockData_Update(D_CAN_RX);
}

// Pointers
uint8_t *nECU_CAN_getPointer_WheelSetup(void) // get pointer to the recived data of wheel setup variable
{
  return &RxData_Frame0[0];
}
uint8_t *nECU_CAN_getPointer_Coolant(void) // get pointer to the recived data of coolant variable
{
  return &RxData_Frame0[1];
}
uint16_t *nECU_CAN_getPointer_RPM(void) // get pointer to the recived data of RPM variable
{
  return (uint16_t *)&RxData_Frame0[2];
}