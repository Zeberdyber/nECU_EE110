/**
 ******************************************************************************
 * @file    nECU_UART.c
 * @brief   This file provides code for user defined UART functions.
 ******************************************************************************
 */

#include "nECU_UART.h"

/* EOF variables */
uint8_t timestamp;
uint8_t delta_counter;

/* Rx UART objects */
static nECU_UART *PC_UART_obj;
static nECU_UART *Immo_UART_obj;

#if TEST_KNOCK_UART == true
bool Triangle_Buffer_firstPart = true;
bool Triangle_initialized = false;
nECU_UART triangle_knock_uart;
uint16_t Triangle_Buffer[KNOCK_DMA_LEN];
uint8_t output_buffer[512];
void Send_Triangle_UART(void) // function to send triangle wave over UART
{
    if (Triangle_initialized == false) // init structure
    {
        nECU_UART_Init(&triangle_knock_uart, &PC_UART, output_buffer);
        for (uint32_t i = 0; i < (KNOCK_DMA_LEN / 2); i++)
        {
            Triangle_Buffer[i] = i;
        }
        for (uint32_t i = 0; i < (KNOCK_DMA_LEN / 2); i++)
        {
            uint8_t current = (KNOCK_DMA_LEN / 2) - i;
            Triangle_Buffer[(KNOCK_DMA_LEN / 2) + i] = current;
        }
        Triangle_initialized = true;
    }
    else
    {
        if (Triangle_Buffer_firstPart)
        {
            nECU_UART_SendKnock(&Triangle_Buffer[0], &triangle_knock_uart);
            Triangle_Buffer_firstPart = false;
        }
        else
        {
            nECU_UART_SendKnock(&Triangle_Buffer[(KNOCK_DMA_LEN / 2) - 1], &triangle_knock_uart);
            Triangle_Buffer_firstPart = true;
        }
    }
}
#endif

/* Knock ADC data transmission */
void nECU_UART_SendKnock(uint16_t *input_buffer, nECU_UART *knock_uart) // send knock data over
{
    knock_uart->length = nECU_UART_KnockSuperFrame(input_buffer, knock_uart->message, UART_ADC_COUNT, DELTA_PREC); // prepare buffer
    nECU_UART_Tx(knock_uart);                                                                                      // send frame
}
uint8_t nECU_UART_KnockSuperFrame(uint16_t *input_buffer, uint8_t *output_buffer, uint16_t input_length, uint8_t delta_bit_count) // compose Super frame (diferential frame), returns resulting frame length
{

    uint16_t output_length = 5 + (((input_length - 1) * delta_bit_count) / 8); // +5 for alpha value and EOF; -1 for alpha value
    if ((((input_length - 1) * delta_bit_count) % 8) > 0)                      // if has a reminder
    {
        output_length++;
    }

    uint16_t maximal_delta_value = 0;
    for (uint8_t i = 0; i < delta_bit_count; i++) // get maximal value for selected bit count of delta
    {
        maximal_delta_value |= 0x1 << delta_bit_count;
    }

    uint8_t temp_buffer[output_length];          // create temporary buffer
    memset(temp_buffer, 0, sizeof(temp_buffer)); // zero-out temporary buffer

    timestamp = nECU_Get_FrameTimer(); // get current timestamp

    ((uint16_t *)(temp_buffer))[0] = input_buffer[0]; // copy first value from input buffer as two first bytes

    /* compresses data to save boudrate */
    uint8_t bitIndex = 0;   // index of current bit in the conversion
    uint16_t byteIndex = 2; // start at third byte (first two are the alpha value)

    for (int i = 0; i < (input_length - 1); i++) // -1 to account for the alpha value
    {
        int16_t delta = input_buffer[i + 1] - input_buffer[i];
        if ((uint16_t)delta > maximal_delta_value) // breaks if delta overflow
        {
            return 0; // error
        }

        for (int j = 0; j < delta_bit_count; j++)
        {
            uint8_t bitCopied = (((delta >> j) & 0x01) << bitIndex) & 0xFF;
            temp_buffer[byteIndex] |= bitCopied;
            bitIndex++;
            if (bitIndex == 8)
            {
                bitIndex = 0;
                byteIndex++;
            }
        }
    }

    temp_buffer[output_length - 3] = delta_counter & 0xFF; // adds frame counter
    temp_buffer[output_length - 2] = timestamp & 0xFF;     // adds timestamp
    temp_buffer[output_length - 1] = END_BYTE;             // adds end byte
    delta_counter++;                                       // increment frame counter

    memcpy(output_buffer, temp_buffer, sizeof(temp_buffer)); // copy data to the output

    return output_length;
}

/* UART interface */
void nECU_UART_Init(nECU_UART *obj, UART_HandleTypeDef *huart, uint8_t *buffer) // initializes structure
{
    obj->huart = huart;
    obj->message = buffer;
    obj->length = 0;
    obj->pending = false;
    if (huart == &PC_UART)
    {
        PC_UART_obj = obj;
    }
    else if (huart == &IMMO_UART)
    {
        Immo_UART_obj = obj;
    }
}
HAL_StatusTypeDef nECU_UART_Tx(nECU_UART *obj) // sends the packet if possible
{
    if (obj->pending == false) // break if no data to send
    {
        return HAL_ERROR;
    }

    if (nECU_UART_Tx_Busy(obj) == true)
    {
        return HAL_BUSY; // breaks function if not possible to send
    }

    if (obj->length == 0) // checks if any data was written to the buffer
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef Tx_status = HAL_UART_Transmit_IT((obj->huart), (obj->message), (obj->length)); // sends the data

    if (Tx_status != HAL_OK)
    {
        return Tx_status;
    }

    // clears the structure
    obj->pending = false;
}
HAL_StatusTypeDef nECU_UART_Rx(nECU_UART *obj) // starts the recive on UART
{
    if (obj->pending == true) // break if there is data in the buffer
    {
        return HAL_ERROR;
    }

    if (nECU_UART_Rx_Busy(obj) == true)
    {
        return HAL_BUSY; // breaks function if not possible to send
    }

    if (obj->length == 0) // checks if length was specified
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef Rx_status = HAL_UART_Receive_IT((obj->huart), (obj->message), (obj->length));

    return Rx_status;
}
HAL_StatusTypeDef nECU_UART_Tx_Abort(nECU_UART *obj) // stops Tx transmission
{
    obj->pending = false;
    obj->length = 0;
    return HAL_UART_AbortTransmit_IT((obj->huart));
}
HAL_StatusTypeDef nECU_UART_Rx_Abort(nECU_UART *obj) // stops Rx transmission
{
    obj->pending = false;
    obj->length = 0;
    return HAL_UART_AbortReceive_IT((obj->huart));
}
bool *nECU_UART_Pending_Flag(nECU_UART *obj) // returns pending flag pointer
{
    return &(obj->pending);
}
bool nECU_UART_Tx_Busy(nECU_UART *obj) // returns if Tx is busy
{
    HAL_UART_StateTypeDef state = HAL_UART_GetState(obj->huart); // gets current state of the peripheral
    if ((state & 0x5) && (state > HAL_UART_STATE_RESET))         // checks 2 bit which indicate: HAL_UART_STATE_BUSY_TX, HAL_UART_STATE_BUSY_TX_RX
    {
        return true;
    }
    return false;
}
bool nECU_UART_Rx_Busy(nECU_UART *obj) // returns if Rx is busy
{
    HAL_UART_StateTypeDef state = HAL_UART_GetState(obj->huart); // gets current state of the peripheral
    if ((state & 0x4) && (state > HAL_UART_STATE_RESET))         // checks 1 bit which indicate: HAL_UART_STATE_BUSY_RX, HAL_UART_STATE_BUSY_TX_RX
    {
        return true;
    }
    return false;
}

/* Callbacks */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // Rx completed
{
    if (huart == &PC_UART)
    {
        PC_UART_obj->pending = true;
        // do the things for PC
    }
    else if (huart == &IMMO_UART)
    {
        Immo_UART_obj->pending = true;
        // do the things for Immo
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) // Tx completed
{
    if (huart == &PC_UART)
    {
        // do the things for PC
    }
    else if (huart == &IMMO_UART)
    {
        // do the things for Immo
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) // Called while UART error
{
    if (huart == &PC_UART)
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
