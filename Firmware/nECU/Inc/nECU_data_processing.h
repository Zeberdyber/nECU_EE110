/**
 ******************************************************************************
 * @file    nECU_data_processing.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_data_processing.c file
 */
#ifndef _NECU_DATA_PROCESSING_H_
#define _NECU_DATA_PROCESSING_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_tim.h"
#include "string.h"
#include "stdbool.h"
#include "stdio.h"

/* Definitions */
#define ADC_MAX_VALUE_12BIT 4095 // Maximum value a 12bit ADC can produce

    /* typedef */

    /* Function Prototypes */
    /* Analog sensors */
    void nECU_calculateLinearCalibration(SensorCalibration *inst);           // function to calculate factor (a) and offset (b) for linear formula: y=ax+b
    float nECU_getLinearSensor(uint16_t ADC_Value, SensorCalibration *inst); // function to get result of linear sensor

    /* Conversion */
    uint64_t nECU_FloatToUint(float in, uint8_t bitCount); // returns float capped to given bitCount. ex: 8bit - 255max, 10bit - 1023max
    int64_t nECU_FloatToInt(float in, uint8_t bitCount);   // returns float capped to given bitCount. ex: 8bit - 127<>-128, 10bit - 511<>-512
    float ADCToVolts(uint16_t ADCValue);
    uint16_t VoltsToADC(float Voltage);

    /* ADC buffer operations */
    void nECU_ADC_AverageDMA(ADC_HandleTypeDef *hadc, uint16_t *inData, uint16_t inLength, uint16_t *outData, float smoothAlpha); // average out dma buffer

    /* Smoothing functions */
    uint16_t nECU_expSmooth(uint16_t *in, uint16_t *in_previous, float alpha);                                           // exponential smoothing algorithm
    uint16_t nECU_averageSmooth(uint16_t *Buffer, uint16_t *in, uint8_t dataLen);                                        // averages whole buffer and adds to the input buffer (FIFO, moving average)
    uint16_t nECU_averageExpSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *in_previous, uint8_t dataLen, float alpha); // exponential smoothing before averaging

    /* Bool <-> Byte */
    void nECU_compressBool(bool *bufferIn, uint8_t *out);   // compress bool array to one byte
    void nECU_decompressBool(uint8_t *in, bool *bufferOut); // decompress byte to bool array

    /* Sensors */
    void nECU_Sensor_Routine(Sensor_Handle *sensor);

    /* Tests */
    bool nECU_DataProcessing_test(bool logging_enable); // Run test
#ifdef __cplusplus
}
#endif

#endif /* _NECU_DATA_PROCESSING_H_ */