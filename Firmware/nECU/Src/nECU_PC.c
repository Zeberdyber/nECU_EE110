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