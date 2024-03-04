/**
 ******************************************************************************
 * @file    nECU_flash.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_flash.c file
 */

#ifndef _NECU_FLASH_H_
#define _NECU_FLASH_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "stm32f4xx.h"
#include "string.h"

/* Definitions */
#define FLASH_DATA_START_ADDRESS 0x080E0000                                                                  // address of sector 11 of flash memory
#define FLASH_DATA_SIZE 16                                                                                   // Size of memory sector in bytes
#define SPEED_DATA_OFFSET 0                                                                                  // offset where to start the speed calibration data in flash
#define USER_SETTINGS_OFFSET (sizeof(nECU_SpeedCalibrationData) + SPEED_DATA_OFFSET)                         // offset where to start the user settings data in flash
#define DEBUG_QUE_OFFSET (sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings) + SPEED_DATA_OFFSET) // offset where to start the debug que data in flash
#define FLASH_MINIMUM_RUN_TIME 1000                                                                          // to allow debugger to work

    /* Function Prototypes */
    /* Speed calibration data functions (flash function interface) */
    void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);
    void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);

    /* User settings data functions (flash function interface) */
    void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF);
    void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF);

    /* Debug que (flash function interface) */
    void nECU_saveDebugQue(nECU_Debug_error_que *que);
    void nECU_readDebugQue(nECU_Debug_error_que *que);

    /* Flash functions */
    void nECU_FLASH_cleanFlashSector(void); // clean flash sector
    void nECU_FLASH_getAllMemory(void);     // get data from flash
    void nECU_FLASH_saveFlashSector(void);  // save everything, then read to RAM

    /* Dedicated to data type functions */
    void nECU_FLASH_writeSpeedCalibrationData(const nECU_SpeedCalibrationData *data); // function to write calibration data to flash memory
    void nECU_FLASH_readSpeedCalibrationData(nECU_SpeedCalibrationData *data);        // function to read calibration data from flash memory
    void nECU_FLASH_writeUserSettings(const nECU_UserSettings *data);                 // function to write settings data to flash memory
    void nECU_FLASH_readUserSettings(nECU_UserSettings *data);                        // function to read settings data to flash memory
    void nECU_FLASH_writeDebugQue(nECU_Debug_error_que *que);                         // function to write debug que to flash memory
    void nECU_FLASH_readDebugQue(nECU_Debug_error_que *que);                          // function to read debug que to flash memory

    /* Helper functions */
    void nECU_compressBool(bool *bufferIn, uint8_t *out);   // compress bool array to one byte
    void nECU_decompressBool(uint8_t *in, bool *bufferOut); // decompress byte to bool array

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */