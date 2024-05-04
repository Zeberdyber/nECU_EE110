/**
 ******************************************************************************
 * @file    nECU_PC.c
 * @brief   This file provides code for communication with PC.
 ******************************************************************************
 */

#include "nECU_PC.h"

static uint16_t cnt = 0; // Licznik wyslanych wiadomosci
uint8_t data[50];        // Tablica przechowujaca wysylana wiadomosc.
uint16_t size = 0;       // Rozmiar wysylanej wiadomosci

bool pc_transmit = false;

/* all pc transmission variables */
static nECU_PC PC;
/* status flags */
static bool PC_Initialized = false, PC_Working = false;

void test_uart(void)
{
    if (HAL_UART_GetState(&huart3) == HAL_UART_STATE_BUSY_TX || pc_transmit == false)
    {
        return;
    }

    ++cnt;                                                             // Zwiekszenie licznika wyslanych wiadomosci.
    size = sprintf(data, "Liczba wyslanych wiadomosci: %d.\n\r", cnt); // Stworzenie wiadomosci do wyslania oraz przypisanie ilosci wysylanych znakow do zmiennej size.
    HAL_UART_Transmit_IT(&huart3, data, size);                         // Rozpoczecie nadawania danych z wykorzystaniem przerwan
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);                      // Zmiana stanu pinu na diodzie LED
}

void nECU_PC_Init(void)
{
    if (PC_Initialized == false)
    {
        /* clear buffers */
        memset((PC.in_buf), 0, PC_UART_BUF_LEN);
        memset((PC.out_buf), 0, PC_UART_BUF_LEN);
        /* Initialize structures */
        nECU_UART_Init(&(PC.input), &PC_UART, (PC.in_buf));
        nECU_UART_Init(&(PC.output), &PC_UART, (PC.out_buf));
        // Onboard_LED_Init_struct(&(PC.Rx_LED), LED1_Pin);
        PC_Initialized = true;
    }
    if (PC_Initialized == true && PC_Working == false)
    {
        PC_Working = true;
    }
}

void nECU_PC_Indicator(void) // function responsible to set state of onboard LED
{
    // this function sets value of the LED, but does force the output

    /* Check the peripheral */
    bool Tx_busy = nECU_UART_Tx_Busy(&(PC.output));
    bool Rx_busy = nECU_UART_Rx_Busy(&(PC.input));
}