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
#define FLASH_DATA_START_ADDRESS 0x080E0000 // address of sector 11 of flash memory
#define FLASH_DATA_SIZE 16                  // Size of memory sector in bytes
#define SPEED_DATA_OFFSET 0                 // offset where to start the speed calibration data in flash
#define USER_SETTINGS_OFFSET 16             // offset where to start the user settings data in flash

    /* typedef */
    typedef struct
    {
        float SpeedSensor1;
        float SpeedSensor2;
        float SpeedSensor3;
        float SpeedSensor4;

    } SpeedCalibrationData;

    typedef struct
    {
        uint8_t boolByte1;
    } UserSettings;

    typedef struct
    {
        SpeedCalibrationData speedData;
        UserSettings userData;
        bool dataInitialized;
    } FlashContent;

    /* Function Prototypes */
    /* Speed calibration data functions (flash function interface) */
    void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);
    void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);
    bool nECU_testSpeedCalibration(void); // test both read and write to flash memory

    /* User settings data functions (flash function interface) */
    void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF);
    void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF);
    bool nECU_testUserSettings(void); // test both read and write to flash memory

    /* Flash functions */
    void nECU_FLASH_cleanFlashSector(void);                                      // clean flash sector
    void nECU_FLASH_getAllMemory(void);                                          // get data from flash
    void nECU_FLASH_saveFlashSector(void);                                       // save everything, then read to RAM
    void nECU_FLASH_writeSpeedCalibrationData(const SpeedCalibrationData *data); // function to write calibration data to flash memory
    void nECU_FLASH_readSpeedCalibrationData(SpeedCalibrationData *data);        // function to read calibration data from flash memory
    void nECU_FLASH_writeUserSettings(const UserSettings *data);                 // function to write settings data to flash memory
    void nECU_FLASH_readUserSettings(UserSettings *data);                        // function to read settings data to flash memory

    /* Helper functions */
    void nECU_compressBool(bool *d0, bool *d1, bool *d2, bool *d3, bool *d4, bool *d5, bool *d6, bool *d7, uint8_t *out);  // compress bools to one byte
    void nECU_decompressBool(uint8_t *in, bool *d0, bool *d1, bool *d2, bool *d3, bool *d4, bool *d5, bool *d6, bool *d7); // decompress byte to bools

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */