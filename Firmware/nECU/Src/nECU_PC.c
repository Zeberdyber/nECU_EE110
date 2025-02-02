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
    static bool first_scan = true;
    if (first_scan)
    {
        static uint32_t start_tick, stop_tick, diff, sign_count;
        nECU_PC_Init();
        start_tick = HAL_GetTick();

        for (uint16_t cnt = 0; cnt < 5000; cnt++)
        {
            sign_count += printf("Liczba wyslanych wiadomosci: %d.\n\r", cnt); // Stworzenie wiadomosci do wyslania oraz przypisanie ilosci wysylanych znakow do zmiennej size.
        }

        stop_tick = HAL_GetTick();
        if (start_tick < stop_tick)
        {
            diff = stop_tick - start_tick;
        }
        else
        {
            diff = stop_tick + start_tick - UINT32_MAX;
        }
        float speed = ((float)sign_count / diff) * (1.170285714285714); // multiplier to account for 1024 bits in a kB and 1 parity bit in UART
        printf("Test trwal %lu tickow, przeslano %lu znakow. Predkosc %d.%d kB/s.\n", diff, sign_count, (int)speed, (int)((speed - (int)speed) * 100));
    }
    first_scan = false;
}

/* General functions */
bool nECU_PC_Start(void) // initializes structures for PC communication over UART
{
    bool status = false;
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
        OnBoard_LED_Start();

        D_PC.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_PC.Status & D_BLOCK_INITIALIZED && !(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_WORKING;
        OnBoard_LED_L_Add_Animation(&(PC.Tx_LED));
        OnBoard_LED_R_Add_Animation(&(PC.Rx_LED));
    }

    return status;
}
bool nECU_PC_Stop(void) // call to stop transmission
{
    bool status = false;
    if (!(D_PC.Status & D_BLOCK_WORKING))
    {
        D_PC.Status |= D_BLOCK_CODE_ERROR;
        return;
    }

    OnBoard_LED_L_Remove_Animation(&(PC.Tx_LED));
    OnBoard_LED_L_Remove_Animation(&(PC.Rx_LED));

    D_PC.Status -= D_BLOCK_INITIALIZED_WORKING;

    return status;
}

/* Send */
int _write(int fd, char *ptr, int len)
{
    fd = fd; // to "use" fd -> make compiler hapy

    static uint8_t i = 0;
    while (PC_UART.gState != HAL_UART_STATE_READY)
    {
        i++; // wait until UART is ready
    }

    memcpy(&(PC.out_buf), ptr, len);
    PC.output.length = len;
    nECU_PC_Transmit();
    return len;
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