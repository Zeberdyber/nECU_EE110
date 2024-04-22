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
#include "nECU_debug.h"

/* Definitions */
#define FLASH_DATA_START_ADDRESS 0x080E0000 // address of sector 11 of flash memory
#define FLASH_DATA_END_ADDRESS 0x080FFFFF   // end address of sector 11 of flash memory
#define FLASH_MINIMUM_RUN_TIME 1000         // to allow debugger to work

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
    void nECU_FLASH_cleanFlashSector(void);       // clean flash sector
    void nECU_FLASH_cleanFlashSector_check(void); // check if erase was successful
    void nECU_FLASH_getAllMemory(void);           // get data from flash
    void nECU_FLASH_saveFlashSector(void);        // save everything, then read to RAM

    /* Dedicated to data type functions */
    void nECU_FLASH_writeSpeedCalibrationData(nECU_SpeedCalibrationData *data); // function to write calibration data to flash memory
    void nECU_FLASH_readSpeedCalibrationData(nECU_SpeedCalibrationData *data);  // function to read calibration data from flash memory
    void nECU_FLASH_checkSpeedCalibrationData(nECU_SpeedCalibrationData *data); // check if passed data is the same as in memory

    void nECU_FLASH_writeUserSettings(nECU_UserSettings *data); // function to write settings data to flash memory
    void nECU_FLASH_readUserSettings(nECU_UserSettings *data);  // function to read settings data to flash memory
    void nECU_FLASH_checkUserSettings(nECU_UserSettings *data); // check if passed data is the same as in memory

    void nECU_FLASH_writeDebugQue(nECU_Debug_error_que *que); // function to write debug que to flash memory
    void nECU_FLASH_readDebugQue(nECU_Debug_error_que *que);  // function to read debug que to flash memory
    void nECU_FLASH_checkDebugQue(nECU_Debug_error_que *que); // check if passed que is the same as in memory

    /* Helper functions */
    void nECU_compressBool(bool *bufferIn, uint8_t *out);   // compress bool array to one byte
    void nECU_decompressBool(uint8_t *in, bool *bufferOut); // decompress byte to bool array

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */