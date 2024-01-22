/**
 ******************************************************************************
 * @file    nECU_stock.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_stock.c file
 */
#ifndef _NECU_STOCK_H_
#define _NECU_STOCK_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_tim.h"
#include "nECU_Knock.h"

    /* Definitions */
#define MAP_kPA_CALIB_MIN 320  // minimum sensor value result for minimum voltage
#define MAP_kPA_CALIB_MAX 1000 // maximum sensor value result for maximum voltage
#define MAP_ADC_CALIB_MIN 1300 // minimum ADC value for minimum sensor value
#define MAP_ADC_CALIB_MAX 2950 // maximum ADC value for maximum sensor value
#define MAP_DECIMAL_POINT 0    // how many numbers after decimal point

#define BACKPRESSURE_kPA_CALIB_MIN -20  // minimum sensor value result for minimum voltage
#define BACKPRESSURE_kPA_CALIB_MAX 0    // maximum sensor value result for maximum voltage
#define BACKPRESSURE_ADC_CALIB_MIN 800  // minimum ADC value for minimum sensor value
#define BACKPRESSURE_ADC_CALIB_MAX 3213 // maximum ADC value for maximum sensor value
#define BACKPRESSURE_DECIMAL_POINT 0    // how many numbers after decimal point

#define OXYGEN_VOLTAGE_CALIB_MIN 0.3 // minimum sensor value result, minimum voltage
#define OXYGEN_VOLTAGE_CALIB_MAX 1.3 // maximum sensor value result, maximum voltage
#define OXYGEN_VOLTAGE_MAX 1.0       // maximum voltage of lambda sensor
#define OXYGEN_VOLTAGE_MIN 0.0       // minimum voltage of lambda sensor
#define OXYGEN_HEATER_MAX 90         // maximum % infill of heater PWM
#define OXYGEN_HEATER_MIN 20         // maximum % infill of heater PWM
#define OXYGEN_COOLANT_MAX 80        // maximum degrees that will couse minimum infill
#define OXYGEN_COOLANT_MIN 20        // minimum degrees that will couse maximum infill
#define OXYGEN_DECIMAL_POINT 2       // how many numbers after decimal point

#define VSS_PULSES_PER_KM 4500 // number of pulses that will be recived for a kilometer traveled

    /* typedef */
    typedef struct
    {
        uint16_t ADC_MeasuredMax, ADC_MeasuredMin;
        float OUT_MeasuredMax, OUT_MeasuredMin;
        float offset, factor;
    } AnalogSensorCalibration;

    typedef struct
    {
        AnalogSensorCalibration calibrationData;
        uint16_t *ADC_input;
        uint16_t decimalPoint;
        float outputFloat;
        uint16_t output16bit;
        uint8_t output8bit;
        bool initialized;
    } AnalogSensor;

    typedef struct
    {
        TIM_HandleTypeDef *Timer; // Timer used for PWM
        uint32_t Channel;         // Timers channel used
        float Infill;             // infill of PWM signal
    } PWM_Out;

    typedef struct
    {
        PWM_Out Heater;
        AnalogSensor sensor;
        uint8_t *Coolant;
        float Infill_max, Infill_min;
        float Coolant_max, Coolant_min;
    } Oxygen;

    typedef struct
    {
        TIM_HandleTypeDef *htim;
        float refClock;
        float period; // in ms
    } TimerStock;

    typedef struct
    {
        TimerStock tim;
        uint32_t VSS_Channel;
        uint32_t VSS_prevCCR;
        float frequency;
        uint8_t Speed;
        uint16_t watchdogCount;
    } VSS_Handle;

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
    /* Oxygen */
    uint8_t *nECU_OX_GetPointer(void); // returns pointer to resulting data
    void nECU_OX_Init(void);           // initialize narrowband lambda structure
    void nECU_OX_Update(void);         // update narrowband lambda structure
    void nECU_OX_DeInit(void);         // deinitialize narrowband lambda structure
    /* VSS - Vehicle Speed Sensor */
    uint8_t *nECU_VSS_GetPointer(void);                // returns pointer to resulting data
    void nECU_VSS_Init(void);                          // initialize VSS structure
    void nECU_VSS_Update(void);                        // update VSS structure
    void nECU_VSS_DeInit(void);                        // deinitialize VSS structure
    void nECU_VSS_DetectZero(TIM_HandleTypeDef *htim); // detect if zero km/h
    /* General */
    void nECU_Stock_Start(void);  // function to initialize all stock stuff
    void nECU_Stock_Stop(void);   // function to deinitialize all stock stuff
    void nECU_Stock_Update(void); // function to update structures

#ifdef __cplusplus
}
#endif

#endif /* _NECU_STOCK_H_ */