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
#include "string.h"
#include "stdbool.h"

/* Definitions */
#define VREFINT_CALIB ((uint16_t *)((uint32_t)0x1FFF7A2A)) // Internal voltage reference raw value at 30 degrees C, VDDA=3.3V (defined in datasheet)
#define VREF_CALIB 3.3                                     // VDDA voltage at which all other values were created (defined in datasheet)
#define ADC_MAX_VALUE_12BIT 4095                           // Maximum value a 12bit ADC can produce

    /* typedef */

    /* Function Prototypes */

    /* Conversion functions */
    float ADCToVolts(uint16_t ADCValue);
    uint16_t VoltsToADC(float Voltage);

    /* ADC buffer operations */
    void nECU_ADC_AverageDMA(ADC_HandleTypeDef *hadc, uint16_t *inData, uint16_t inLength, uint16_t *outData, float smoothAlpha); // average out dma buffer

    /* Smoothing functions */
    void nECU_expSmooth(uint16_t *in, uint16_t *in_previous, uint16_t *out, float alpha);                    // exponential smoothing algorithm
    void nECU_averageSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen);                 // averages whole buffer and adds to the input buffer (FIFO, moving average)
    void nECU_averageExpSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen, float alpha); // exponential smoothing before averaging

    /* Bool <-> Byte */
    void nECU_compressBool(bool *bufferIn, uint8_t *out);   // compress bool array to one byte
    void nECU_decompressBool(uint8_t *in, bool *bufferOut); // decompress byte to bool array

#ifdef __cplusplus
}
#endif

#endif /* _NECU_DATA_PROCESSING_H_ */