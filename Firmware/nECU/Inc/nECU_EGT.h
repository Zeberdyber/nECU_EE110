/**
 ******************************************************************************
 * @file    nECU_EGT.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_EGT.c file
 */

#ifndef _NECU_EGT_H_
#define _NECU_EGT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "spi.h"
#include "nECU_spi.h"
#include "gpio.h"
#include "nECU_debug.h"

/* Definitions */
#define EGT_DECIMAL_POINT 0     // Specifies how many numbers should be after decimal point
#define EGT_NEGATIVE_OFFSET 100 // How much to subtract from the result
#define EGT_COMM_FAIL_MAX 15    // how many communication can fail befor error will be flagged
#define EGT_STARTUP_DELAY 250   // time in ms for MAX31855 startup

    /* Function Prototypes */
    uint16_t *nECU_EGT_Temperature_getPointer(EGT_Sensor_ID ID);  // get function that returns pointer to output data of sensor, ready for can transmission
    int16_t *nECU_EGT_TemperatureIC_getPointer(EGT_Sensor_ID ID); // get function that returns pointer to internal temperature data of sensor
    EGT_Error_Code *nECU_EGT_Error_getPointer(EGT_Sensor_ID ID);  // get function returns pointer to error code

    bool EGT_Start(void);   // initialize all sensors and start communication
    void EGT_Stop(void);    // stop the routines
    void EGT_Routine(void); // periodic function to be called every main loop execution

    static void nECU_EGT_Convert_All(void);             // convert data if pending
    static void EGT_TemperatureTo10bit(MAX31855 *inst); // function to convert temperature value to 10bit number for CAN transmission
    void nECU_EGT_Callback(void);                       // callback from SPI_TX end callback
    void nECU_EGT_Error_Callback(void);                 // Callback after SPI communication fail
    void EGT_RequestUpdate(void);                       // indicate that update is needed

    static bool MAX31855_Init(MAX31855 *inst, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin); // First initialization
    static void MAX31855_collectError(MAX31855 *inst);                                 // get current error value
    static void MAX31855_UpdateSimple(MAX31855 *inst, SPI_HandleTypeDef *hspi);        // Recive data over SPI and convert it into struct, dont use while in DMA mode
    static void MAX31855_ConvertData(MAX31855 *inst);                                  // For internal use bit decoding and data interpretation

#ifdef __cplusplus
}
#endif

#endif /* _NECU_EGT_H_ */