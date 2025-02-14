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

// Console colors
#define Console_Color_Off "\[\033[0m\]" // Text Format Reset

// Regular Colors
#define Console_Black "\[\033[0;30m\]"  // Black
#define Console_Red "\[\033[0;31m\]"    // Red
#define Console_Green "\[\033[0;32m\]"  // Green
#define Console_Yellow "\[\033[0;33m\]" // Yellow
#define Console_Blue "\[\033[0;34m\]"   // Blue
#define Console_Purple "\[\033[0;35m\]" // Purple
#define Console_Cyan "\[\033[0;36m\]"   // Cyan
#define Console_White "\[\033[0;37m\]"  // White

// Bold Colors
#define Console_BBlack "\[\033[1;30m\]"  // Bold Black
#define Console_BRed "\[\033[1;31m\]"    // Bold Red
#define Console_BGreen "\[\033[1;32m\]"  // Bold Green
#define Console_BYellow "\[\033[1;33m\]" // Bold Yellow
#define Console_BBlue "\[\033[1;34m\]"   // Bold Blue
#define Console_BPurple "\[\033[1;35m\]" // Bold Purple
#define Console_BCyan "\[\033[1;36m\]"   // Bold Cyan
#define Console_BWhite "\[\033[1;37m\]"  // Bold White

// Underline Colors
#define Console_UBlack "\[\033[4;30m\]"  // Underlined Black
#define Console_URed "\[\033[4;31m\]"    // Underlined Red
#define Console_UGreen "\[\033[4;32m\]"  // Underlined Green
#define Console_UYellow "\[\033[4;33m\]" // Underlined Yellow
#define Console_UBlue "\[\033[4;34m\]"   // Underlined Blue
#define Console_UPurple "\[\033[4;35m\]" // Underlined Purple
#define Console_UCyan "\[\033[4;36m\]"   // Underlined Cyan
#define Console_UWhite "\[\033[4;37m\]"  // Underlined White

// Background
#define Console_On_Black "\[\033[40m\]"  // Background Black
#define Console_On_Red "\[\033[41m\]"    // Background Red
#define Console_On_Green "\[\033[42m\]"  // Background Green
#define Console_On_Yellow "\[\033[43m\]" // Background Yellow
#define Console_On_Blue "\[\033[44m\]"   // Background Blue
#define Console_On_Purple "\[\033[45m\]" // Background Purple
#define Console_On_Cyan "\[\033[46m\]"   // Background Cyan
#define Console_On_White "\[\033[47m\]"  // Background White

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
    void nECU_console_progressBar(char *bar, uint8_t len, uint8_t percent);

    /* Callbacks */
    void nECU_PC_Tx_Start_Callback(void); // to be called when Tx from PC has started
    void nECU_PC_Tx_Stop_Callback(void);  // to be called when Tx from PC is done
    void nECU_PC_Rx_Start_Callback(void); // to be called when Rx to PC has started
    void nECU_PC_Rx_Stop_Callback(void);  // to be called when Rx to PC is done

#ifdef __cplusplus
}
#endif

#endif /* _NECU_PC_H_ */