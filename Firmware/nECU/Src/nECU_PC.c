/**
 ******************************************************************************
 * @file    nECU_PC.c
 * @brief   This file provides code for communication with PC.
 ******************************************************************************
 */

#include "nECU_PC.h"

/* all pc transmission variables */
static nECU_PC PC = {0};
/* status flags */
extern nECU_ProgramBlockData D_PC; // diagnostic and flow control data

void test_uart(void) // test function only
{
    static uint16_t cnt = 0; // Licznik wyslanych wiadomosci
    nECU_PC_Init();
    nECU_PC_Start();
    PC.output.length = sprintf(&(PC.out_buf[0]), "Liczba wyslanych wiadomosci: %d.\n\r", cnt); // Stworzenie wiadomosci do wyslania oraz przypisanie ilosci wysylanych znakow do zmiennej size.
    nECU_PC_Transmit();
    ++cnt; // Zwiekszenie licznika wyslanych wiadomosci.
}

/* General functions */
void nECU_PC_Init(void) // initializes structures for PC communication over UART
{
    if (D_PC.Status == D_BLOCK_STOP)
    {
        /* clear buffers */
        memset((PC.in_buf), 0, PC_UART_BUF_LEN);
        memset((PC.out_buf), 0, PC_UART_BUF_LEN);
        /* Initialize structures */
        nECU_UART_Init(&(PC.input), &PC_UART, (PC.in_buf));
        nECU_UART_Init(&(PC.output), &PC_UART, (PC.out_buf));
        OnBoard_LED_Animation_Init(&(PC.Tx_LED), LED_ANIMATE_UART_ID);
        OnBoard_LED_Animation_Init(&(PC.Rx_LED), LED_ANIMATE_UART_ID);

        D_PC.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_PC.Status & D_BLOCK_INITIALIZED)
    {
        nECU_PC_Start();
        D_PC.Status |= D_BLOCK_WORKING;
    }
}
void nECU_PC_Start(void) // call to start transmission
{
    if (!(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_CODE_ERROR;
        return;
    }

    OnBoard_LED_L_Add_Animation(&(PC.Tx_LED));
    OnBoard_LED_R_Add_Animation(&(PC.Rx_LED));
}
void nECU_PC_Stop(void) // call to stop transmission
{
    if (!(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_CODE_ERROR;
        return;
    }

    OnBoard_LED_L_Remove_Animation(&(PC.Tx_LED));
    OnBoard_LED_L_Remove_Animation(&(PC.Rx_LED));

    D_PC.Status = D_BLOCK_STOP;
}

/* Flow control */
static void nECU_PC_Transmit(void) // call to send a frame
{
    if (!(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_CODE_ERROR;
        return;
    }
    PC.output.pending = true;
    nECU_PC_Tx_Start_Callback();
    nECU_UART_Tx(&(PC.output));
    nECU_Debug_ProgramBlockData_Update(&D_PC);
}
static void nECU_PC_Recieve(void) // call to start listening for frames
{
    if (!(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_CODE_ERROR;
        return;
    }
    nECU_PC_Rx_Start_Callback();
    nECU_UART_Rx(&(PC.input));
    nECU_Debug_ProgramBlockData_Update(&D_PC);
}
/* Callbacks */
void nECU_PC_Tx_Start_Callback(void) // to be called when Tx from PC has started
{
    OnBoard_LED_Animation_BlinkStart(&(PC.Tx_LED), 50, 1);
}
void nECU_PC_Tx_Stop_Callback(void) // to be called when Tx from PC is done
{
    OnBoard_LED_Animation_BlinkStart(&(PC.Tx_LED), 50, 1);
}
void nECU_PC_Rx_Start_Callback(void) // to be called when Rx to PC has started
{
    OnBoard_LED_Animation_BlinkStart(&(PC.Rx_LED), 50, 1);
}
void nECU_PC_Rx_Stop_Callback(void) // to be called when Rx to PC is done
{
    OnBoard_LED_Animation_BlinkStart(&(PC.Rx_LED), 50, 1);
}