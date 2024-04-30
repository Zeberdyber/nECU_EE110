/**
 ******************************************************************************
 * @file    nECU_UART.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_UART.c file
 */
#ifndef _NECU_UART_H_
#define _NECU_UART_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "usart.h"
#include "stdbool.h"
#include "nECU_adc.h"
#include "string.h"
#include "nECU_tim.h"
#include "nECU_main.h"

/* Definitions */
#define END_BYTE 0xFF         // what will be reciving software looking for to determine end of frame
#define TEST_KNOCK_UART false // enables uart test mode, trinagluar wave sent over uart
#define SEND_KNOCK_RAW false  // eneables RAW ADC data send

/* Definitions  Delta*/
#define DELTA_PREC 16      // precision of delta (number of bits in delta output)
#define UART_ADC_COUNT 256 // number of ADC conversions to be in one UART frame

#define PC_UART huart3   // peripheral connected to the PC
#define IMMO_UART huart1 // peripheral connected to the immobilizer

#if TEST_KNOCK_UART == true
    void Send_Triangle_UART(void); // function to send triangle wave over UART
#endif

    /* Function Prototypes */
    /* Knock ADC data transmission */
    void nECU_UART_SendKnock(uint16_t *input_buffer, nECU_UART *knock_uart);                                                           // send knock data over
    uint8_t nECU_UART_KnockSuperFrame(uint16_t *input_buffer, uint8_t *output_buffer, uint16_t input_length, uint8_t delta_bit_count); // compose Super frame (diferential frame), returns resulting frame length
    /* UART interface */
    void nECU_UART_Init(nECU_UART *obj, UART_HandleTypeDef *huart, uint8_t *buffer); // initializes structure
    HAL_StatusTypeDef nECU_UART_Tx(nECU_UART *obj);                                  // sends the packet if possible
    HAL_StatusTypeDef nECU_UART_Rx(nECU_UART *obj);                                  // starts the recive on UART
    HAL_StatusTypeDef nECU_UART_Tx_Abort(nECU_UART *obj);                            // stops Tx transmission
    HAL_StatusTypeDef nECU_UART_Rx_Abort(nECU_UART *obj);                            // stops Rx transmission
    bool *nECU_UART_Pending_Flag(nECU_UART *obj);                                    // returns pending flag pointer
    /* Callbacks */
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart); // Rx completed
    void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart); // Tx completed
    void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);  // Called while UART error

#ifdef __cplusplus
}
#endif

#endif /* _NECU_UART_H_ */