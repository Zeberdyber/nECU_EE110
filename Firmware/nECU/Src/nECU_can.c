/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          timing.
 ******************************************************************************
 */

#include "nECU_can.h"
#include "nECU_tests.h"

// TX Data
static nECU_CAN_Tx_Data Tx_frame_List[CAN_TX_ID_MAX] = {0};
static uint32_t const TX_delay_List[CAN_TX_ID_MAX] = {
    [CAN_TX_Speed_ID] = 100,
    [CAN_TX_EGT_ID] = 100,
    [CAN_TX_Stock_ID] = 10,
};
static uint32_t const TX_Msg_ID_List[CAN_TX_ID_MAX] = {
    [CAN_TX_Speed_ID] = 0x500,
    [CAN_TX_EGT_ID] = 0x501,
    [CAN_TX_Stock_ID] = 0x502,
};

// RX Data
static nECU_CAN_Rx_Data Rx_frame_List[CAN_RX_ID_MAX] = {0};
static uint32_t const RX_Msg_ID_List[CAN_RX_ID_MAX] = {
    [CAN_RX_Wheel_ID] = 0x510,
    [CAN_RX_Coolant_ID] = 0x511,
    [CAN_RX_RPM_ID] = 0x512,
};

// General functions
bool nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  bool status_TX = false, status_RX = false;

  if (!nECU_FlowControl_Initialize_Check(D_CAN_TX) && status_TX == false)
  {
    for (nECU_CAN_TX_Frame_ID currentID = 0; currentID < CAN_TX_ID_MAX; currentID++)
      status_TX |= nECU_CAN_TX_Init(currentID);
    if (!status_TX)
      status_TX |= !nECU_FlowControl_Initialize_Do(D_CAN_TX);
  }
  if (!nECU_FlowControl_Initialize_Check(D_CAN_RX) && status_RX == false)
  {
    for (nECU_CAN_RX_Frame_ID currentID = 0; currentID < CAN_RX_ID_MAX; currentID++)
      status_RX |= nECU_CAN_RX_Init(currentID);
    if (!status_RX)
      status_RX |= !nECU_FlowControl_Initialize_Do(D_CAN_RX);
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

  if (!nECU_FlowControl_Working_Check(D_CAN_RX) && status_RX == false)
  {
    if (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_READY)
      status_RX |= (HAL_CAN_Start(&hcan1) != HAL_OK);
    if (!status_RX)
    {
      status_RX |= !nECU_FlowControl_Working_Do(D_CAN_RX);
      status_RX |= (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK); // Initialize CAN Bus Rx Interrupt
    }
  }
  if (status_RX)
  {
    nECU_FlowControl_Error_Do(D_CAN_RX);
  }

  return status_TX | status_RX;
}
void nECU_CAN_WriteToBuffer(nECU_CAN_TX_Frame_ID frameID, uint8_t size) // copy input data to corresponding frame buffer
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) || (frameID > CAN_TX_ID_MAX) || size == 0)
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    return;
  }

  if (size > 8) // cap max length
    size = 8;

  Tx_frame_List[frameID].can_data.Header.DLC = size;
  memcpy(Tx_frame_List[frameID].can_data.Buffer,
         Tx_frame_List[frameID].buf.Buffer,
         Tx_frame_List[frameID].can_data.Header.DLC); // copy data to can buffer
}
bool nECU_CAN_Stop(void) // stop all CAN code, with timing
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_CAN_TX) && status == false)
  {
    for (nECU_CAN_TX_Frame_ID currenID = 0; currenID < CAN_TX_ID_MAX; currenID++)
      status |= nECU_Delay_Stop(&(Tx_frame_List[currenID].frame_delay));
    if (!status)
      status |= nECU_FlowControl_Stop_Do(D_CAN_TX);
  }
  if (nECU_FlowControl_Working_Check(D_CAN_RX) && status == false)
  {
    status |= (HAL_OK != HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING)); // Stop waiting for RX
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_CAN_RX);
  }

  status |= (HAL_OK != HAL_CAN_Stop(&hcan1));

  return status;
}

// TX functions
void nECU_CAN_TX_CheckTime(void) // checks if it is time to send packet
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    return;
  }
  for (nECU_CAN_TX_Frame_ID currentID = 0; currentID < CAN_TX_ID_MAX; currentID++)
  {
    if (nECU_FlowControl_Working_Check(D_Frame_Speed_ID + currentID)) // check if frame working
    {
      nECU_Delay_Update(&(Tx_frame_List[currentID].frame_delay)); // update time
      if (Tx_frame_List[currentID].frame_delay.done)              // check if time is above threshold
      {
        nECU_Delay_Start(&(Tx_frame_List[currentID].frame_delay)); // restart delay

        if (nECU_CAN_TX_TransmitFrame(currentID))
          nECU_FlowControl_Error_Do(D_Frame_Speed_ID + currentID);
        else
          nECU_Frame_TX_done(currentID);
      }
    }
  }

  nECU_Debug_ProgramBlockData_Update(D_CAN_TX);
}
static bool nECU_CAN_TX_Init(nECU_CAN_TX_Frame_ID currentID)
{
  if (currentID > CAN_TX_ID_MAX) // Check if in bounds
    return true;                 // Break

  bool status = false;
  // delay
  status |= nECU_Delay_Set(&(Tx_frame_List[currentID].frame_delay), TX_delay_List[currentID]);
  status |= nECU_Delay_Start(&(Tx_frame_List[currentID].frame_delay));
  // default values
  Tx_frame_List[currentID].can_data.Header.StdId = TX_Msg_ID_List[currentID];
  Tx_frame_List[currentID].can_data.Header.IDE = CAN_ID_STD;
  Tx_frame_List[currentID].can_data.Header.RTR = CAN_RTR_DATA;
  memset(Tx_frame_List[currentID].can_data.Buffer, 0, sizeof(Tx_frame_List[currentID].can_data.Buffer)); // clear buffer
  // data pointer
  if (nECU_Frame_getPointer(currentID))
    Tx_frame_List[currentID].buf.Buffer = nECU_Frame_getPointer(currentID);
  else
    status |= true;

  return status;
}
static bool nECU_CAN_TX_TransmitFrame(nECU_CAN_TX_Frame_ID frameID) // send selected frame over CAN
{
  bool status = false;
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) || (frameID > CAN_TX_ID_MAX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    status |= true;
    return status;
  }

  uint32_t mail = nECU_CAN_TX_FindMailbox(&hcan1);
  status |= (HAL_CAN_AddTxMessage(&hcan1, &(Tx_frame_List[frameID].can_data.Header), Tx_frame_List[frameID].can_data.Buffer, &(mail)) != HAL_OK); // Transmit the data

  return status;
}
static uint32_t nECU_CAN_TX_FindMailbox(CAN_HandleTypeDef *hcan) // will find empty mailbox
{
  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan))                                                     // check if any mailbox is empty
    for (uint32_t Mailbox = CAN_TX_MAILBOX0; Mailbox <= CAN_TX_MAILBOX2; Mailbox = Mailbox << 1) // go threw all mailboxes
      if (!HAL_CAN_IsTxMessagePending(&hcan1, Mailbox))                                          // check mailbox if empty
        return Mailbox;

  return 8; // return non-valid mailbox
}

// RX functions
static bool nECU_CAN_RX_Init(nECU_CAN_RX_Frame_ID currentID)
{
  if (currentID >= CAN_RX_ID_MAX)
    return true; // Break

  bool status = false;
  Rx_frame_List[currentID].filter.FilterBank = currentID;
  Rx_frame_List[currentID].filter.FilterMode = CAN_FILTERMODE_IDMASK;
  Rx_frame_List[currentID].filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  Rx_frame_List[currentID].filter.FilterIdHigh = RX_Msg_ID_List[currentID] << 5;
  Rx_frame_List[currentID].filter.FilterIdLow = 0;
  Rx_frame_List[currentID].filter.FilterMaskIdHigh = 0xFFFF;
  Rx_frame_List[currentID].filter.FilterMaskIdLow = 0xFFFF;
  Rx_frame_List[currentID].filter.FilterScale = CAN_FILTERSCALE_32BIT;
  Rx_frame_List[currentID].filter.FilterActivation = ENABLE;
  Rx_frame_List[currentID].filter.SlaveStartFilterBank = 0;

  status |= (HAL_CAN_ConfigFilter(&hcan1, &(Rx_frame_List[currentID].filter)) != HAL_OK); // Initialize CAN Filter
  return status;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // interrupt callback when new Rx frame in FIFO0
{
  if (!nECU_FlowControl_Working_Check(D_CAN_RX))
    return; // break
  static CAN_RxHeaderTypeDef RX_Header;
  static uint8_t Buffer[8] = {0};
  memset(Buffer, 0, sizeof(Buffer));                            // clear buffer
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RX_Header, Buffer); // Receive CAN bus message to canRX buffer
  nECU_CAN_RX_Frame_ID ID = nECU_CAN_RX_Identify(&RX_Header);
  nECU_CAN_RX_Update(ID, Buffer);
  nECU_Debug_ProgramBlockData_Update(D_CAN_RX);
}
static nECU_CAN_RX_Frame_ID nECU_CAN_RX_Identify(CAN_RxHeaderTypeDef *pHeader)
{
  for (nECU_CAN_RX_Frame_ID currentID = 0; currentID < CAN_RX_ID_MAX; currentID++)
    if (RX_Msg_ID_List[currentID] == (pHeader->StdId))
      return currentID;

  return CAN_RX_ID_MAX;
}
static void nECU_CAN_RX_Update(nECU_CAN_RX_Frame_ID currentID, uint8_t *buf) // update value
{
  if (currentID > CAN_RX_ID_MAX)
    return; // Break

  union Int32ToBytes Converter;
  memcpy(Converter.byteArray, buf, 4);
  Rx_frame_List[currentID].output = Converter.IntValue;
}
int32_t nECU_CAN_RX_getValue(nECU_CAN_RX_Frame_ID currentID) // returns value of given frame
{
  if (currentID > CAN_RX_ID_MAX)
    return 0; // Break
  return Rx_frame_List[currentID].output;
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
