/**
 ******************************************************************************
 * @file    nECU_data_processing.c
 * @brief   This file provides code for data processing like smoothing etc.
 ******************************************************************************
 */

#include "nECU_data_processing.h"

/* Conversion functions */
float ADCToVolts(uint16_t ADCValue)
{
    return (VREF_CALIB * ADCValue) / ADC_MAX_VALUE_12BIT;
}
uint16_t VoltsToADC(float Voltage)
{
    Voltage *= ADC_MAX_VALUE_12BIT;
    Voltage /= VREF_CALIB;
    return (uint16_t)Voltage;
}

/* ADC buffer operations */
void nECU_ADC_AverageDMA(ADC_HandleTypeDef *hadc, uint16_t *inData, uint16_t inLength, uint16_t *outData, float smoothAlpha) // average out dma buffer
{
    uint32_t numChannels = hadc->Init.NbrOfConversion;
    uint32_t avgSum[numChannels];  // create buffer for sum values
    uint16_t avgData[numChannels]; // create temporary buffer for smoothing

    memset(avgSum, 0, sizeof(avgSum)); // Clear buffer

    // Sum up all values for each channel
    for (uint16_t convCount = 0; convCount < inLength; convCount += numChannels) // increment per full conversions
    {
        for (uint8_t convChannel = 0; convChannel < numChannels; convChannel++) // go threw each measurement
        {
            avgSum[convChannel] += inData[convCount + convChannel]; // add up new measurement
        }
    }

    // Take an average and smooth
    for (uint8_t Channel = 0; Channel < numChannels; Channel++)
    {
        avgData[Channel] = avgSum[Channel] / (inLength / numChannels);                        // average out
        nECU_expSmooth(&avgData[Channel], &outData[Channel], &outData[Channel], smoothAlpha); // smooth
    }
}

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

/* Bool <-> Byte */
void nECU_compressBool(bool *bufferIn, uint8_t *out) // compress bool array to one byte
{
    // zero-out output
    *out = 0;

    // fill output with data
    for (uint8_t i = 0; i < 8; i++)
    {
        *out |= (bufferIn[i] & 1) << i;
    }
}
void nECU_decompressBool(uint8_t *in, bool *bufferOut) // decompress byte to bool array
{
    for (uint8_t i = 0; i < 8; i++)
    {
        bufferOut[i] = ((*in) >> i) & 1; // copy to outputs
    }
}
