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
    void test_uart(void);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_PC_H_ */