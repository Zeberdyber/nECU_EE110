/**
 ******************************************************************************
 * @file    nECU_Speed.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_Speed.c file
 */

#ifndef _NECU_SPEED_H_
#define _NECU_SPEED_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_adc.h"
#include "nECU_flash.h"
#include "nECU_can.h"

/* Definitions */
#define Wheel_Circ_Set_1 1888   // Circumference of wheel for setup 1 in mm
#define Wheel_Circ_Set_2 1870   // Circumference of wheel for setup 2 in mm
#define Speed_DecimalPoint 1    // Specifies how many numbers should be after decimal point
#define Speed_HzTomVolts 0.63   // Specifies electrical parameter of LM2907 used components
#define Speed_ToothCount 43     // Impulses per revolution of a wheel
#define Speed_SampleNumber 10   // Number of samples to average from
#define SPEED_OFFSET 4          // negative offset for noise compensation
#define SPEED_EXP_ALPHA 0.04    // alpha of exponential filter
#define SPEED_CALIB_VELOCITY 50 // speed at which calibraion is done in km/h

    /* Interface functions */
    uint16_t *Speed_GetSpeed(Speed_Sensor_ID ID);     // get current speed
    uint16_t *Speed_GetSpeedSlow(Speed_Sensor_ID ID); // get slower (average) speed
    void Speed_SetWheelSetup(uint8_t WheelSetup);     // set wheel setup for all sensors

    /* Averaging */
    void Speed_AverageCalc(Speed_Sensor *Sensor); // routine to calculate averaged speed
    void Speed_AverageInit(Speed_Sensor *Sensor); // Initialize averaging structure

    /* General functions */
    bool Speed_Start(void);                          // function to start Speed function set
    void Speed_Update(void);                         // perform update of all sensors
    void Speed_SensorUpdate(Speed_Sensor *Sensor);   // update one sensors data
    void Speed_CorrectToCalib(Speed_Sensor *Sensor); // correct data to calibration multiplier
    void Speed_ADCToSpeed(Speed_Sensor *Sensor);     // function to convert RAW ADC data to real speed in km/h

    /* Calibration functions */
    void Speed_CalibrateSingle(Speed_Sensor *Sensor); // function to generate calibration multiplier
    void Speed_CalibrateAll(void);                    // function to calibrate speed sensors (periodic function)
    void Speed_CalibrateInit(void);                   // initialize calibration structure
    void Speed_CalibrateStart(void);                  // start calibration process

    /* Periodic functions */
    void Speed_TimingEvent(void); // function to be called periodicaly with desired data update rate

#ifdef __cplusplus
}
#endif

#endif /* _NECU_SPEED_H_ */