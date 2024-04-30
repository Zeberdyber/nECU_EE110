/**
 ******************************************************************************
 * @file    nECU_UART.c
 * @brief   This file provides code for user defined UART functions.
 ******************************************************************************
 */

#include "nECU_UART.h"

/* UART peripheral definition */
extern UART_HandleTypeDef huart3;
#define PC_UART &huart3

/* Flow control bits */
uint8_t Rx_buffer[1];
bool Knock_UART_Transmission = false;
bool DMA_Done = true;
extern bool pc_transmit;

/* UART Tx Buffer for knock RAW data */
uint8_t knock_UART_buffer[UART_KNOCK_LEN_BYTES];

/* EOF variables */
uint8_t timestamp;
uint8_t delta_counter;

#if TEST_UART == 1
bool Triangle_Buffer_ready = false;
bool Triangle_Buffer_firstPart = true;
uint16_t Triangle_Buffer[KNOCK_DMA_LEN];
void Send_Triangle_UART(void) // function to send triangle wave over UART
{
    if (Triangle_Buffer_ready == false) // create test data (triangle wave)
    {
        for (uint32_t i = 0; i < (KNOCK_DMA_LEN / 2); i++)
        {
            Triangle_Buffer[i] = i;
        }
        for (uint32_t i = 0; i < (KNOCK_DMA_LEN / 2); i++)
        {
            uint8_t current = (KNOCK_DMA_LEN / 2) - i;
            Triangle_Buffer[(KNOCK_DMA_LEN / 2) + i] = current;
        }
        Triangle_Buffer_ready = true;
    }
    else
    {
        if (Triangle_Buffer_firstPart)
        {
            nECU_UART_SendKnock(&Triangle_Buffer[0]);
            Triangle_Buffer_firstPart = false;
        }
        else
        {
            nECU_UART_SendKnock(&Triangle_Buffer[(KNOCK_DMA_LEN / 2) - 1]);
            Triangle_Buffer_firstPart = true;
        }
    }
}
#endif
/* Uart tests */

/* TX functions */
void nECU_UART_SuperFrame(uint16_t *input_buffer, uint8_t *output_buffer) // compose Super frame (diferential frame)
{
    uint8_t temp_buffer[UART_KNOCK_LEN_BYTES];

    memset(temp_buffer, 0, sizeof(temp_buffer));

    timestamp = nECU_Get_FrameTimer(); // get current timestamp

    ((uint16_t *)(temp_buffer))[0] = input_buffer[0];

    /* USE THIS PART ONLY WHEN SPARE CPU TIME, STILL IN TEST STAGE*/
    bool delta_calc_new = true;
    if (delta_calc_new == true)
    {
        uint8_t bitIndex = 0;
        uint16_t byteIndex = 2;
        for (int i = 0; i < (UART_ADC_COUNT - 1); i++)
        {
            int32_t delta = input_buffer[i + 1] - input_buffer[i];
            for (int j = 0; j < DELTA_PREC; j++)
            {
                uint8_t bitCopied = (((delta >> j) & 0x01) << bitIndex) & 0xFF;
                temp_buffer[byteIndex] |= bitCopied;
                bitIndex++;
                if (bitIndex == 8)
                {
                    bitIndex = 0;
                    byteIndex++;
                    if (byteIndex >= DELTA_ADDR_BYTE_EOF) // safety falback if overflow
                    {
                        break;
                    }
                }
            }
        }
    }
    else // works only for DELTA_PREC == 8
    {
        for (int i = 2; i < DELTA_ADDR_BYTE_EOF; i++)
        {
            int8_t delta = input_buffer[i] - input_buffer[i - 1];
            temp_buffer[i] = delta;
        }
    }

    // for (uint16_t i = 0; i < UART_ADC_COUNT; i++)
    // {
    //     ((uint16_t *)(temp_buffer))[i] = input_buffer[i];
    // }

    temp_buffer[DELTA_ADDR_BYTE_EOF] = delta_counter & 0xFF;
    temp_buffer[DELTA_ADDR_BYTE_EOF + 1] = timestamp & 0xFF;
    temp_buffer[DELTA_ADDR_BYTE_EOF + 2] = END_BYTE;
    delta_counter++;

    memcpy(output_buffer, temp_buffer, sizeof(temp_buffer));
}
void nECU_UART_SendKnock(uint16_t *input_buffer) // send knock data over
{
    if (Knock_UART_Transmission == false) // check activation bit
    {
        return;
    }
    nECU_UART_SuperFrame(input_buffer, knock_UART_buffer);
    nECU_UART_DMA_Tx_Knock();
}
void nECU_UART_DMA_Tx_Knock(void) // send knock data in DMA mode
{
    if (HAL_UART_STATE_BUSY_TX == HAL_UART_GetState(PC_UART) || HAL_UART_STATE_BUSY_TX_RX == HAL_UART_GetState(PC_UART)) // safety fallback in case of TX busy
    {
        delta_counter++; // increase to indicate data loss
        return;
    }
    if (DMA_Done == false)
    {
        HAL_UART_DMAStop(PC_UART); // stop dma to transmit current frame
    }
    HAL_UART_Transmit_DMA(PC_UART, knock_UART_buffer, UART_KNOCK_LEN_BYTES);
    DMA_Done = false;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) // Tx completed
{
    if (huart == PC_UART)
    {
        HAL_UART_DMAStop(PC_UART); // stops DMA at the end of transmission
        DMA_Done = true;
    }
}

/* RX functions */
void nECU_UART_RXStartPC(void) // initialize rx from UART communication
{
    Rx_buffer[0] = 0;
    HAL_UART_Receive_IT(PC_UART, Rx_buffer, 1);
}
void nECU_UART_RXStopPC(void) // Stop rx from UART communication
{
    HAL_UART_AbortReceive_IT(PC_UART);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // Rx completed
{
    if (huart == PC_UART)
    {
        /* Decode recived byte */
        if (Rx_buffer[0] == 0xEE)
        {
            nECU_UART_Tx_Stop_Routine();
        }
        else if (Rx_buffer[0] == 0xFF)
        {
            nECU_UART_Tx_Start_Routine();
        }

        /* Turn UART to listen */
        nECU_UART_RXStartPC();
    }
}

/* nECU control while UART Tx active*/
void nECU_UART_Tx_Start_Routine(void) // prepare nECU to be able to transmit
{
    pc_transmit = true;
    // nECU_Stop();
    // ADC3_START();
    // nECU_UART_RXStartPC();
    // Knock_UART_Transmission = true;
}
void nECU_UART_Tx_Stop_Routine(void) // return back to regular execution
{
    pc_transmit = false;
    // nECU_Start();
    // Knock_UART_Transmission = false;
}
bool *nECU_UART_KnockTx(void) // return pointer to knock tx flag
{
    return &Knock_UART_Transmission;
}

/* Error detection */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) // Called while UART error
{
    if (huart = PC_UART)
    {
        uint16_t error = HAL_UART_ERROR_NONE;
        error = HAL_UART_GetError(huart);
        if (error != HAL_UART_ERROR_NONE)
        {
            /* Implement error handling here */
            UNUSED(huart); // do nothing
        }
    }
}
