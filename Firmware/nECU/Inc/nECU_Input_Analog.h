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
#include "nECU_debug.h"

/* Definitions */
#define INTERNAL_TEMP_SLOPE (float)2.5   // slope defined in datasheet [mV/C]
#define INTERNAL_TEMP_V25 (float)0.76    // Voltage at 25C from calibration (defined in datasheet)
#define INTERNAL_TEMP_STARTUP_DELAY 1000 // to skip first couple readings
#define INTERNAL_TEMP_UPDATE_DELAY 1000  // once per second (1000ms)
#define INTERNAL_TEMP_MULTIPLIER 100     // value by which internal temperature result will be multipled

#define MAP_kPA_CALIB_MIN 270  // minimum sensor value result for minimum voltage
#define MAP_kPA_CALIB_MAX 1020 // maximum sensor value result for maximum voltage
#define MAP_ADC_CALIB_MIN 1203 // minimum ADC value for minimum sensor value
#define MAP_ADC_CALIB_MAX 3020 // maximum ADC value for maximum sensor value

#define BACKPRESSURE_kPA_CALIB_MIN -20  // minimum sensor value result for minimum voltage
#define BACKPRESSURE_kPA_CALIB_MAX 0    // maximum sensor value result for maximum voltage
#define BACKPRESSURE_ADC_CALIB_MIN 800  // minimum ADC value for minimum sensor value
#define BACKPRESSURE_ADC_CALIB_MAX 3213 // maximum ADC value for maximum sensor value
#define BACKPRESSURE_DECIMAL_POINT 0    // how many numbers after decimal point

    /* typedef */

    /* Function Prototypes */
    /* Internal Temperatre (MCU) */
    bool nECU_InternalTemp_Start(void);              // initialize structure
    void nECU_InternalTemp_Callback(void);           // run when conversion ended
    void nECU_InternalTemp_Routine(void);            // perform update of output variables
    int16_t *nECU_InternalTemp_getTemperature(void); // return current temperature pointer (multiplied 100x)

    /* MAP */
    uint16_t *nECU_MAP_GetPointer(void); // returns pointer to resulting data
    bool nECU_MAP_Start(void);           // initialize MAP structure
    void nECU_MAP_Routine(void);         // update MAP structure
    /* BackPressure */
    uint8_t *nECU_BackPressure_GetPointer(void); // returns pointer to resulting data
    bool nECU_BackPressure_Start(void);          // initialize BackPressure structure
    void nECU_BackPressure_Routine(void);        // update BackPressure structure

    void nECU_A_Input_Init_All(void);                                                                                                                       // Initialize additional analog inputs
    void nECU_A_Input_Update_All(void);                                                                                                                     // Update additional analog inputs
    bool nECU_A_Input_Init(AnalogSensor_Handle *sensor, uint16_t inMax, uint16_t inMin, float outMax, float outMin, uint16_t *ADC_Data);                    // function to setup the structure
    void nECU_A_Input_SetSmoothing(AnalogSensor_Handle *sensor, float alpha, uint16_t *smoothing_buffer, uint8_t buffer_length, uint16_t update_frequency); // setups filtering and smoothing
    void nECU_A_Input_Update(AnalogSensor_Handle *sensor);                                                                                                  // update current value

    /* General */
    void nECU_Analog_Start(void);  // start of all analog functions
    void nECU_Analog_Stop(void);   // stop of all analog functions
    void nECU_Analog_Update(void); // update to all analog functions

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_ANALOG_H_ */