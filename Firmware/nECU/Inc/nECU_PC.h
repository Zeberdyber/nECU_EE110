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
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

    /* typedef */

    /* Function Prototypes */
    void test_uart(void); // test function only

    /* General functions */
    bool nECU_PC_Start(void);
    bool nECU_PC_Stop(void);
    void nECU_PC_Routine(void);

    /* Flow control */
    static void nECU_PC_Transmit(void); // call to send a frame
    static void nECU_PC_Recieve(void);  // call to start listening for frames
    /* Send */
    // PUTCHAR_PROTOTYPE;
    int _write(int fd, char *ptr, int len);
    /* Callbacks */
    void nECU_PC_Tx_Start_Callback(void); // to be called when Tx from PC has started
    void nECU_PC_Tx_Stop_Callback(void);  // to be called when Tx from PC is done
    void nECU_PC_Rx_Start_Callback(void); // to be called when Rx to PC has started
    void nECU_PC_Rx_Stop_Callback(void);  // to be called when Rx to PC is done

#ifdef __cplusplus
}
#endif

#endif /* _NECU_PC_H_ */