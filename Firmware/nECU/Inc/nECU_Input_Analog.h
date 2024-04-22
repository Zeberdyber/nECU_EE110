/**
 ******************************************************************************
 * @file    nECU_Input_Analog.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_Input_Analog.c file
 */
#ifndef _NECU_INPUT_ANALOG_H_
#define _NECU_INPUT_ANALOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_tim.h"

    /* Definitions */
#define MAP_kPA_CALIB_MIN 270  // minimum sensor value result for minimum voltage
#define MAP_kPA_CALIB_MAX 1020 // maximum sensor value result for maximum voltage
#define MAP_ADC_CALIB_MIN 1203 // minimum ADC value for minimum sensor value
#define MAP_ADC_CALIB_MAX 3020 // maximum ADC value for maximum sensor value
#define MAP_DECIMAL_POINT 0    // how many numbers after decimal point

#define BACKPRESSURE_kPA_CALIB_MIN -20  // minimum sensor value result for minimum voltage
#define BACKPRESSURE_kPA_CALIB_MAX 0    // maximum sensor value result for maximum voltage
#define BACKPRESSURE_ADC_CALIB_MIN 800  // minimum ADC value for minimum sensor value
#define BACKPRESSURE_ADC_CALIB_MAX 3213 // maximum ADC value for maximum sensor value
#define BACKPRESSURE_DECIMAL_POINT 0    // how many numbers after decimal point

    /* typedef */

    /* Function Prototypes */
    /* Analog sensors */
    void nECU_calculateLinearCalibration(AnalogSensorCalibration *inst);                // function to calculate factor (a) and offset (b) for linear formula: y=ax+b
    float nECU_getLinearSensor(uint16_t *ADC_Value, AnalogSensorCalibration *inst);     // function to get result of linear sensor
    uint16_t nECU_FloatToUint16_t(float input, uint8_t decimalPoint, uint8_t bitCount); // convert float value to uint16_t value with correct decimal point and bit lenght
    uint8_t nECU_FloatToUint8_t(float input, uint8_t decimalPoint, uint8_t bitCount);   // convert float value to uint8_t value with correct decimal point and bit lenght
    /* MAP */
    uint16_t *nECU_MAP_GetPointer(void); // returns pointer to resulting data
    void nECU_MAP_Init(void);            // initialize MAP structure
    void nECU_MAP_Update(void);          // update MAP structure
    /* BackPressure */
    uint8_t *nECU_BackPressure_GetPointer(void); // returns pointer to resulting data
    void nECU_BackPressure_Init(void);           // initialize BackPressure structure
    void nECU_BackPressure_Update(void);         // update BackPressure structure
    /* General */
    void nECU_Analog_Start(void);  // start of all analog functions
    void nECU_Analog_Stop(void);   // stop of all analog functions
    void nECU_Analog_Update(void); // update to all analog functions

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_ANALOG_H_ */