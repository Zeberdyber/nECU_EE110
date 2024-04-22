/**
 ******************************************************************************
 * @file    nECU_data_processing.c
 * @brief   This file provides code for data processing like smoothing etc.
 ******************************************************************************
 */

#include "nECU_data_processing.h"

/* Smoothing functions */
void nECU_expSmooth(uint16_t *in, uint16_t *out, float alpha) // exponential smoothing algorithm
{
    *out = (*in * alpha) + (*out * (1 - alpha));
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
    nECU_expSmooth(in, &exp_out, alpha);
    nECU_averageSmooth(Buffer, &exp_out, out, dataLen);
}

/* test functions */
bool nECU_smoothing_tests(void) // tests all smoothing algorithms
{
    if (nECU_expSmooth_test() == false)
    {
        return false;
    }
    if (nECU_averageSmooth_test() == false)
    {
        return false;
    }
    if (nECU_averageExpSmooth_test() == false)
    {
        return false;
    }
    return true;
}
bool nECU_expSmooth_test(void) // tests nECU_expSmooth() function
{
    uint16_t A, B;
    float alpha;

    A = 100;
    B = 0;
    alpha = 0.5;
    nECU_expSmooth(&A, &B, alpha);
    if (B != 50)
    {
        return false;
    }
    A = 0;
    B = 100;
    alpha = 0.5;
    nECU_expSmooth(&A, &B, alpha);
    if (B != 50)
    {
        return false;
    }
    A = 100;
    B = 200;
    alpha = 0.2;
    nECU_expSmooth(&A, &B, alpha);
    if (B != 180)
    {
        return false;
    }
    A = 100;
    B = 200;
    alpha = 0.8;
    nECU_expSmooth(&A, &B, alpha);
    if (B != 120)
    {
        return false;
    }
    return true;
}
bool nECU_averageSmooth_test(void) // tests nECU_averageSmooth() function
{
    uint16_t buffer[10];
    uint8_t bufferLen = 10;
    for (uint8_t i = 0; i < 2 * 10; i += 2) // fills buffer with integers == {0,2,4,6,8,10,12,14,16,18}
    {
        buffer[i / 2] = i;
    }
    uint16_t A, B;
    A = 0;
    B = 0;
    nECU_averageSmooth(buffer, &A, &B, bufferLen);
    // buffer should have values == {2,4,6,8,10,12,14,16,18,0}
    if (buffer[bufferLen - 1] != A) // check if last value matches
    {
        return false;
    }
    if (B != 9) // check if result correct
    {
        return false;
    }
    A = 22;
    nECU_averageSmooth(buffer, &A, &B, bufferLen);
    // buffer should have values == {4,6,8,10,12,14,16,18,0,22}
    if (B != 11) // check if result correct
    {
        return false;
    }
    return true;
}
bool nECU_averageExpSmooth_test(void) // tests nECU_averageExpSmooth() function
{
    uint16_t buffer[10];
    uint8_t bufferLen = 10;
    for (uint8_t i = 0; i < 2 * 10; i += 2) // fills buffer with integers == {0,2,4,6,8,10,12,14,16,18}
    {
        buffer[i / 2] = i;
    }
    uint16_t A, B;
    float alpha = 0.5;
    A = 12;
    B = 24;
    nECU_averageExpSmooth(buffer, &A, &B, bufferLen, alpha);
    // buffer should have values == {2,4,6,8,10,12,14,16,18,18}
    if (B != 10) // check if result correct
    {
        return false;
    }
    A = 128;
    alpha = 0.8;
    nECU_averageExpSmooth(buffer, &A, &B, bufferLen, alpha);
    // buffer should have values == {4,6,8,10,12,14,16,18,18,104}
    if (B != 21) // check if result correct
    {
        return false;
    }
    return true;
}