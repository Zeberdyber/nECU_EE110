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

/* Definitions */
#define EGT_DECIMAL_POINT 0         // Specifies how many numbers should be after decimal point
#define EGT_NEGATIVE_OFFSET 100     // How much to subtract from the result
#define EGT_MAXIMUM_PENDING_COUNT 5 // How often at least the data have to be converted

    /* Function Prototypes */
    MAX31855 *EGT_IdentifyID(EGT_Sensor_ID ID);                                                          // returns pointer to appropriete structure
    uint16_t *EGT_GetTemperaturePointer(EGT_Sensor_ID ID);                                               // get function that returns pointer to output data of sensor, ready for can transmission
    uint16_t *EGT_GetTemperatureInternalPointer(EGT_Sensor_ID ID);                                       // get function that returns pointer to internal temperature data of sensor
    bool *EGT_GetInitialized(void);                                                                      // get function to check if code was EGT_Initialized
    bool *EGT_GetUpdateOngoing(void);                                                                    // get function to check if current comunication is ongoing
    EGT_Error_Code *EGT_GetErrorState(EGT_Sensor_ID ID);                                                 // get function returns pointer to error code
    void EGT_Start(void);                                                                                // initialize all sensors and start communication
    void EGT_GetSPIData(bool error);                                                                     // get data of all sensors
    void EGT_ConvertAll(void);                                                                           // convert data if pending
    void EGT_PeriodicEventHP(void);                                                                      // high priority periodic event, launched from timer interrupt
    void EGT_PeriodicEventLP(void);                                                                      // low priority periodic event, launched from regular main loop
    void EGT_TemperatureTo10bit(MAX31855 *inst);                                                         // function to convert temperature value to 10bit number for CAN transmission
    void MAX31855_Init(MAX31855 *inst, SPI_HandleTypeDef *hspi, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin); // First initialization
    void MAX31855_collectError(MAX31855 *inst);                                                          // get current error value
    void MAX31855_UpdateSimple(MAX31855 *inst);                                                          // Recive data over SPI and convert it into struct, dont use while in DMA mode
    void MAX31855_ConvertData(MAX31855 *inst);                                                           // For internal use bit decoding and data interpretation

#ifdef __cplusplus
}
#endif

#endif /* _NECU_EGT_H_ */