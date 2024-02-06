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
#include "stdbool.h"
#include "stm32f4xx.h"
#include "string.h"

/* Definitions */
#define FLASH_DATA_START_ADDRESS 0x080E0000                                          // address of sector 11 of flash memory
#define FLASH_DATA_SIZE 16                                                           // Size of memory sector in bytes
#define SPEED_DATA_OFFSET 0                                                          // offset where to start the speed calibration data in flash
#define USER_SETTINGS_OFFSET (sizeof(nECU_SpeedCalibrationData) + SPEED_DATA_OFFSET) // offset where to start the user settings data in flash
#define FLASH_MINIMUM_RUN_TIME 1000                                                  // to allow debugger to work

    /* typedef */
    typedef struct
    {
        float SpeedSensor1;
        float SpeedSensor2;
        float SpeedSensor3;
        float SpeedSensor4;

    } nECU_SpeedCalibrationData;

    typedef struct
    {
        uint8_t boolByte1;
    } nECU_UserSettings;

    typedef struct
    {
        nECU_SpeedCalibrationData speedData;
        nECU_UserSettings userData;
        bool dataInitialized;
    } nECU_FlashContent;

    /* Function Prototypes */
    /* Speed calibration data functions (flash function interface) */
    void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);
    void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);

    /* User settings data functions (flash function interface) */
    void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF);
    void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF);

    /* Flash functions */
    void nECU_FLASH_cleanFlashSector(void);                                           // clean flash sector
    void nECU_FLASH_getAllMemory(void);                                               // get data from flash
    void nECU_FLASH_saveFlashSector(void);                                            // save everything, then read to RAM
    void nECU_FLASH_writeSpeedCalibrationData(const nECU_SpeedCalibrationData *data); // function to write calibration data to flash memory
    void nECU_FLASH_readSpeedCalibrationData(nECU_SpeedCalibrationData *data);        // function to read calibration data from flash memory
    void nECU_FLASH_writeUserSettings(const nECU_UserSettings *data);                 // function to write settings data to flash memory
    void nECU_FLASH_readUserSettings(nECU_UserSettings *data);                        // function to read settings data to flash memory

    /* Helper functions */
    void nECU_compressBool(bool *bufferIn, uint8_t *out);   // compress bool array to one byte
    void nECU_decompressBool(uint8_t *in, bool *bufferOut); // decompress byte to bool array

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */