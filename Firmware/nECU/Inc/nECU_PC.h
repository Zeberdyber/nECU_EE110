/**
 ******************************************************************************
 * @file    nECU_PC.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_PC.c file
 */
#ifndef _NECU_PC_H_
#define _NECU_PC_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes */
#include "main.h"
#include "stdio.h"
#include "stdbool.h"
#include "nECU_types.h"
#include "nECU_UART.h"

/* Definitions */
#define PC_INDICATOR_SPEED 5 // how fast will LED blink in Hz

    /* typedef */

    /* Function Prototypes */
    void test_uart(void); // test function only
    /* General functions */
    void nECU_PC_Init(void);  // initializes structures for PC communication over UART
    void nECU_PC_Start(void); // call to start transmission
    void nECU_PC_Stop(void);  // call to stop transmission
    /* Flow control */
    static void nECU_PC_Transmit(void); // call to send a frame
    static void nECU_PC_Recieve(void);  // call to start listening for frames
    /* Callbacks */
    void nECU_PC_Tx_Start_Callback(void); // to be called when Tx from PC has started
    void nECU_PC_Tx_Stop_Callback(void);  // to be called when Tx from PC is done
    void nECU_PC_Rx_Start_Callback(void); // to be called when Rx to PC has started
    void nECU_PC_Rx_Stop_Callback(void);  // to be called when Rx to PC is done

#ifdef __cplusplus
}
#endif

#endif /* _NECU_PC_H_ */