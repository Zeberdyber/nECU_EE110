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
#define END_BYTE 0xFF // what will be reciving software looking for to determine end of frame
#define TEST_UART 0   // enables uart test mode, trinagluar wave sent over uart

/* Definitions  Delta*/
#define ALPHA_LEN 16                                                                                                                   // in bits
#define DELTA_PREC 16                                                                                                                  // precision of delta (number of bits in delta output)
#define UART_ADC_COUNT 256                                                                                                             // number of ADC conversions to be in one UART frame
#define LEN_BOOL_NO_ROUND ((UART_ADC_COUNT - 1) * DELTA_PREC + ALPHA_LEN + 24)                                                         // -1 for Alpha variable, +24 for EOF
#define DELTA_LEN_BOOL (LEN_BOOL_NO_ROUND + (8 - (LEN_BOOL_NO_ROUND % 8)) * (((LEN_BOOL_NO_ROUND + 7) / 8) - (LEN_BOOL_NO_ROUND / 8))) // !!! rounded bit number of recived frame
#define UART_KNOCK_LEN_BYTES (DELTA_LEN_BOOL / 8)                                                                                      // total lenght of recived correct frame
#define DELTA_ADDR_BYTE_EOF (UART_KNOCK_LEN_BYTES - 3)                                                                                 // position where End Of Frame starts

#if TEST_UART == 1
#define TEST_DATA_LENGTH UART_ADC_COUNT * 2
    void Send_Triangle_UART(void); // function to send triangle wave over UART
#endif

    /* typedef */

    /* Function Prototypes */
    void nECU_UART_SuperFrame(uint16_t *input_buffer, uint8_t *output_buffer); // compose Super frame (diferential frame)
    void nECU_UART_SendKnock(uint16_t *input_buffer);                          // send knock data over
    void nECU_UART_DMA_Tx_Knock(void);                                         // send knock data in DMA mode
    void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);                   // Tx completed
    void nECU_UART_RXStartPC(void);                                            // initialize Rx from UART communication
    void nECU_UART_RXStopPC(void);                                             // Stop rx from UART communication
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);                   // Rx completed
    void nECU_UART_Tx_Start_Routine(void);                                     // prepare nECU to be able to transmit
    void nECU_UART_Tx_Stop_Routine(void);                                      // return back to regular execution
    void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);                    // Called while UART error

#ifdef __cplusplus
}
#endif

#endif /* _NECU_UART_H_ */