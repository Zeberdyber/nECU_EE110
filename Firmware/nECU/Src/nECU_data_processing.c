/**
 ******************************************************************************
 * @file    nECU_data_processing.c
 * @brief   This file provides code for data processing like smoothing etc.
 ******************************************************************************
 */

#include "nECU_data_processing.h"

/* Smoothing functions */
void nECU_expSmooth(uint16_t *in, uint16_t *in_previous, uint16_t *out, float alpha) // exponential smoothing algorithm
{
    *out = (*in * alpha) + (*in_previous * (1 - alpha));
}
void nECU_averageSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen) // averages whole buffer and adds to the input buffer (FIFO, moving average)
{

    for (uint8_t i = 0; i < (dataLen - 1); i++) // shift data in the buffer
    {
        Buffer[i] = Buffer[i + 1];
    }
    Buffer[dataLen - 1] = *in; // add new data to the buffer

    uint32_t sum = 0;
    for (uint8_t i = 0; i < dataLen; i++) // sum all input data
    {
        sum += Buffer[i];
    }

    *out = sum / dataLen; // produce output
}
void nECU_averageExpSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen, float alpha) // exponential smoothing before averaging
{
    uint16_t exp_out = *out;
    nECU_expSmooth(in, &exp_out, &exp_out, alpha);
    nECU_averageSmooth(Buffer, &exp_out, out, dataLen);
}