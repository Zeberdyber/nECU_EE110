/**
 ******************************************************************************
 * @file    nECU_PC.c
 * @brief   This file provides code for communication with PC.
 ******************************************************************************
 */

#include "nECU_PC.h"

/* all pc transmission variables */
static nECU_PC PC = {0};

void test_uart(void) // test function only
{
    static bool first_scan = true;
    if (first_scan)
    {
        static uint32_t start_tick, stop_tick, diff, sign_count;
        nECU_PC_Start();
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
        printf("Test trwal %lu tickow, przeslano %lu znakow. Predkosc %d.%d kB/s.\n\r", diff, sign_count, (int)speed, (int)((speed - (int)speed) * 100));
    }
    first_scan = false;
}

/* General functions */
bool nECU_PC_Start(void) // initializes structures for PC communication over UART
{
    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_PC))
    {
        status |= nECU_UART_Init(&(PC.input), &PC_UART, (PC.in_buf));
        status |= nECU_UART_Init(&(PC.output), &PC_UART, (PC.out_buf));
        status |= OnBoard_LED_Start();
        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_PC);
    }
    if (!nECU_FlowControl_Working_Check(D_PC) && status == false)
    {
        OnBoard_LED_L_Add_Animation(&(PC.Tx_LED));
        OnBoard_LED_R_Add_Animation(&(PC.Rx_LED));

        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_PC);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_PC);
    return status;
}
bool nECU_PC_Stop(void) // call to stop transmission
{
    bool status = false;

    if (nECU_FlowControl_Working_Check(D_PC) && status == false)
    {
        OnBoard_LED_L_Remove_Animation(&(PC.Tx_LED));
        OnBoard_LED_L_Remove_Animation(&(PC.Rx_LED));
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_PC);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_PC);

    return status;
}
void nECU_PC_Routine(void)
{
    if (!nECU_FlowControl_Working_Check(D_PC)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_PC);
        return; // Break
    }

    nECU_Debug_ProgramBlockData_Update(D_PC);
}

/* Send */
int _write(int fd, char *ptr, int len) // For printf implementation
{
    fd = fd; // to "use" fd -> make compiler happy

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

/* Console functionalities */
void nECU_console_progressBar(char *bar, uint8_t len, uint8_t percent)
{
    static const uint8_t perc_disp = 6; // length of '(100%)' part in chars
    static char perc[3];                // place holder for percent string

    if (percent > 100) // cap at 100%
        percent = 100;

    memset(bar, ' ', len); // clear memory

    uint8_t res = ((len - perc_disp) * percent) / 100; // calculate how much should be filled

    for (uint8_t l = 0; l < res; l++) // fill bar
        bar[l] = '#';

    uint8_t perc_len = sprintf(perc, "%u", percent); // create percent string

    sprintf(&bar[len - perc_disp], "(   %%)"); // create percent bracket
    for (uint8_t i = perc_len; i > 0; i--)     // copy string to bracket
        bar[len - 2 - i] = perc[perc_len - i];
}

/* Flow control */
static void nECU_PC_Transmit(void) // call to send a frame
{
    if (!nECU_FlowControl_Working_Check(D_PC)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_PC);
        return; // Break
    }

    PC.output.pending = true;
    nECU_PC_Tx_Start_Callback();
    nECU_UART_Tx(&(PC.output));

    nECU_Debug_ProgramBlockData_Update(D_PC);
}
static void nECU_PC_Recieve(void) // call to start listening for frames
{
    if (!nECU_FlowControl_Working_Check(D_PC)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_PC);
        return; // Break
    }

    nECU_PC_Rx_Start_Callback();
    nECU_UART_Rx(&(PC.input));

    nECU_Debug_ProgramBlockData_Update(D_PC);
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