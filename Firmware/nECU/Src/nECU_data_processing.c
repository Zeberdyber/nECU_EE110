/**
 ******************************************************************************
 * @file    nECU_data_processing.c
 * @brief   This file provides code for data processing like smoothing etc.
 ******************************************************************************
 */

#include "nECU_data_processing.h"
/* Analog sensors */
void nECU_calculateLinearCalibration(AnalogSensorCalibration *inst) // function to calculate factor (a) and offset (b) for linear formula: y=ax+b
{
    if (inst == NULL) // break if pointer does not exist
        return;

    inst->factor = (float)(inst->OUT_MeasuredMax - inst->OUT_MeasuredMin) / (inst->ADC_MeasuredMax - inst->ADC_MeasuredMin);
    inst->offset = (float)inst->OUT_MeasuredMax - (inst->factor * inst->ADC_MeasuredMax);
}
float nECU_getLinearSensor(uint16_t ADC_Value, AnalogSensorCalibration *inst) // function to get result of linear sensor
{
    if (inst == NULL) // break if pointer does not exist
        return 0.0;

    return ADC_Value * inst->factor + inst->offset;
}

/* Conversion */
uint64_t nECU_FloatToUint(float in, uint8_t bitCount) // returns float capped to given bitCount. ex: 8bit - 255max, 10bit - 1023max
{
    uint64_t max_val = 0;
    while (bitCount > 0)
    {
        max_val << 1;
        max_val++;
    }

    uint64_t out = in;
    if (out > max_val)
        out = max_val;
    else if (out < 0)
        out = 0;

    return out;
}
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
    if (hadc == NULL || inData == NULL || outData == NULL) // break if pointer does not exist
        return;

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
        outData[Channel] = nECU_expSmooth(&avgData[Channel], &outData[Channel], smoothAlpha); // smooth
    }
}

/* Smoothing functions */
uint16_t nECU_expSmooth(uint16_t *in, uint16_t *in_previous, float alpha) // exponential smoothing algorithm
{
    if (in == NULL || in_previous == NULL) // break if pointer does not exist
        return 0;

    return (*in * alpha) + (*in_previous * (1 - alpha));
}
uint16_t nECU_averageSmooth(uint16_t *Buffer, uint16_t *in, uint8_t dataLen) // averages whole buffer and adds to the input buffer (FIFO, moving average)
{
    if (Buffer == NULL || in == NULL) // break if pointer does not exist
        return 0;

    for (uint8_t i = 0; i < (dataLen - 1); i++) // shift data in the buffer
        Buffer[i] = Buffer[i + 1];

    Buffer[dataLen - 1] = *in; // add new data to the buffer

    uint32_t sum = 0;
    for (uint8_t i = 0; i < dataLen; i++) // sum all input data
        sum += Buffer[i];

    return (sum / dataLen); // produce output
}
uint16_t nECU_averageExpSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *in_previous, uint8_t dataLen, float alpha) // exponential smoothing before averaging
{
    if (Buffer == NULL || in == NULL) // break if pointer does not exist
        return 0;

    uint16_t avg_out = nECU_averageSmooth(Buffer, in, dataLen);

    return nECU_expSmooth(&avg_out, in_previous, alpha);
}

/* Bool <-> Byte */
void nECU_compressBool(bool *bufferIn, uint8_t *out) // compress bool array to one byte
{
    // zero-out output
    *out = 0;

    // fill output with data
    for (uint8_t i = 0; i < 8; i++)
        *out |= (bufferIn[i] & 1) << i;
}
void nECU_decompressBool(uint8_t *in, bool *bufferOut) // decompress byte to bool array
{
    for (uint8_t i = 0; i < 8; i++)
        bufferOut[i] = ((*in) >> i) & 1; // copy to outputs
}

/* Tests */
static bool nECU_DataProcessing_test_FloatToUint(void) // test nECU_FloatToUint()
{
    if (nECU_FloatToUint(15.3, 8) != 15)
        return false;

    if (nECU_FloatToUint(15.5, 8) != 16)
        return false;

    if (nECU_FloatToUint(300, 8) != UINT8_MAX)
        return false;

    if (nECU_FloatToUint(-8.0, 8) != 0)
        return false;

    if (nECU_FloatToUint(2050.15, 11) != 0)
        return false;

    if (nECU_FloatToUint(2050.15, 2) != 3)
        return false;

    return true;
}
static bool nECU_DataProcessing_test_ADC_AverageDMA(void) // test nECU_ADC_AverageDMA()
{
    ADC_HandleTypeDef testADC;
    testADC.Init.NbrOfConversion = 2;

    uint16_t inputBuf[] = {10, 0, 20, 1, 30, 3, 40, 32768, 10, 0, 20, 1, 30, 3, 40, 32768};
    uint16_t outputBuf[] = {0, 0, 0, 0}; // tool large buffer to test data spilage

    nECU_ADC_AverageDMA(&testADC, inputBuf, 16, &outputBuf[1], 1); // no smoothing

    // check limits for spilage
    if (outputBuf[0] != 0)
        return false;
    if (outputBuf[3] != 0)
        return false;

    // check for correct answers
    if (outputBuf[1] != 25)
        return false;
    if (outputBuf[2] != 8193)
        return false;

    return true;
}
static bool nECU_DataProcessing_test_expSmooth(void) // test nECU_expSmooth()
{
    uint16_t A, B;
    float alpha;

    A = 100;
    B = 0;
    alpha = 0.5;
    B = nECU_expSmooth(&A, &B, alpha);
    if (B != 50)
        return false;

    A = 0;
    B = 100;
    alpha = 0.5;
    B = nECU_expSmooth(&A, &B, alpha);
    if (B != 50)
        return false;

    A = 100;
    B = 200;
    alpha = 0.2;
    B = nECU_expSmooth(&A, &B, alpha);
    if (B != 180)
        return false;

    A = 100;
    B = 200;
    alpha = 0.8;
    B = nECU_expSmooth(&A, &B, alpha);
    if (B != 120)
        return false;

    return true;
}
static bool nECU_DataProcessing_test_averageSmooth(void) // test nECU_averageSmooth()
{
    uint16_t buffer[10];
    uint8_t bufferLen = 10;
    for (uint8_t i = 0; i < 2 * 10; i += 2) // fills buffer with integers == {0,2,4,6,8,10,12,14,16,18}
        buffer[i / 2] = i;

    uint16_t A, B;
    A = 0;
    B = 0;
    B = nECU_averageSmooth(buffer, &A, bufferLen);
    // buffer should have values == {2,4,6,8,10,12,14,16,18,0}
    if (buffer[bufferLen - 1] != A) // check if last value matches
        return false;

    if (B != 9) // check if result correct
        return false;

    A = 22;
    B = nECU_averageSmooth(buffer, &A, bufferLen);
    // buffer should have values == {4,6,8,10,12,14,16,18,0,22}
    if (B != 11) // check if result correct
        return false;

    return true;
}
static bool nECU_DataProcessing_test_compdecompBool(void) // test nECU_compressBool() and nECU_decompressBool()
{
    bool bufferIn[8] = {true, true, false, false, true, false, true, false};
    uint8_t byte = 0;
    nECU_compressBool(bufferIn, &byte);
    if (byte != 0b11001010) // check compression result
        return false;

    bool bufferOut[8];
    nECU_decompressBool(&byte, bufferOut);
    for (uint8_t i = 0; i < 8; i++) // check decompression result
    {
        if (bufferIn[i] != bufferOut[i])
            return false;
    }
    return true;
}
bool nECU_DataProcessing_test(bool logging_enable) // Run test
{
    if (logging_enable)
        printf("Started test of 'nECU_data_processing.c\n\r");

    if (!nECU_DataProcessing_test_FloatToUint())
    {
        if (logging_enable)
            printf("\n\rFAIL on nECU_DataProcessing_test_FloatToUint()\n\r");
        return false;
    }
    if (!nECU_DataProcessing_test_ADC_AverageDMA())
    {
        if (logging_enable)
            printf("\n\rFAIL on nECU_DataProcessing_test_ADC_AverageDMA()\n\r");
        return false;
    }
    if (!nECU_DataProcessing_test_expSmooth())
    {
        if (logging_enable)
            printf("\n\rFAIL on nECU_DataProcessing_test_expSmooth()\n\r");
        return false;
    }
    if (!nECU_DataProcessing_test_averageSmooth())
    {
        if (logging_enable)
            printf("\n\rFAIL on nECU_DataProcessing_test_averageSmooth()\n\r");
        return false;
    }
    if (!nECU_DataProcessing_test_compdecompBool())
    {
        if (logging_enable)
            printf("\n\rFAIL on nECU_DataProcessing_test_compdecompBool()\n\r");
        return false;
    }

    if (logging_enable)
        printf("OK\n\r");
    return true;
}