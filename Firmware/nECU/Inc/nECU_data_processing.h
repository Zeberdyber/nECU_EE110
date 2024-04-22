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
#include "stdbool.h"

    /* Definitions */

    /* typedef */

    /* Function Prototypes */
    /* Smoothing functions */
    void nECU_expSmooth(uint16_t *in, uint16_t *out, float alpha);                                           // exponential smoothing algorithm
    void nECU_averageSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen);                 // averages whole buffer and adds to the input buffer (FIFO, moving average)
    void nECU_averageExpSmooth(uint16_t *Buffer, uint16_t *in, uint16_t *out, uint8_t dataLen, float alpha); // exponential smoothing before averaging
    /* test functions */
    bool nECU_smoothing_tests(void);       // tests all smoothing algorithms
    bool nECU_expSmooth_test(void);        // tests nECU_expSmooth() function
    bool nECU_averageSmooth_test(void);    // tests nECU_averageSmooth() function
    bool nECU_averageExpSmooth_test(void); // tests nECU_averageExpSmooth() function

#ifdef __cplusplus
}
#endif

#endif /* _NECU_DATA_PROCESSING_H_ */