/**
 ******************************************************************************
 * @file    nECU_flash.c
 * @brief   This file provides code for user defined flash functions and data
 *          frame forming.
 ******************************************************************************
 */

#include "nECU_flash.h"

FlashContent Flash;

/* Speed calibration data functions (flash function interface) */
void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (Flash.dataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy config if needed to buffer
    if (Flash.speedData.SpeedSensor1 == *Sensor1)
    {
        Flash.speedData.SpeedSensor1 = *Sensor1;
    }
    if (Flash.speedData.SpeedSensor2 == *Sensor1)
    {
        Flash.speedData.SpeedSensor2 = *Sensor1;
    }
    if (Flash.speedData.SpeedSensor3 == *Sensor1)
    {
        Flash.speedData.SpeedSensor3 = *Sensor1;
    }
    if (Flash.speedData.SpeedSensor4 == *Sensor1)
    {
        Flash.speedData.SpeedSensor4 = *Sensor1;
    }

    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (Flash.dataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy data to outputs
    *Sensor1 = Flash.speedData.SpeedSensor1;
    *Sensor2 = Flash.speedData.SpeedSensor2;
    *Sensor3 = Flash.speedData.SpeedSensor3;
    *Sensor4 = Flash.speedData.SpeedSensor4;
}
bool nECU_testSpeedCalibration(void) // test both read and write to flash memory
{
    float T1 = 1.0, T2 = 9.99, T3 = -0.0015, T4 = 181516.2;
    nECU_saveSpeedCalibration(&T1, &T1, &T1, &T4);
    float R1, R2, R3, R4;
    nECU_readSpeedCalibration(&R1, &R2, &R3, &R4);
    if (R1 == T1 && R2 == T2 && R3 == T3 && R4 == T4)
    {
        return true;
    }
    return false;
}

/* User settings data functions (flash function interface) */
void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (Flash.dataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // compress data to bytes
    bool zero = 0;
    nECU_compressBool(pAntiLag, pTractionOFF, &zero, &zero, &zero, &zero, &zero, &zero, &(Flash.userData.boolByte1));
    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (Flash.dataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // decompress to output
    bool zero = 0;
    nECU_decompressBool(&(Flash.userData.boolByte1), pAntiLag, pTractionOFF, &zero, &zero, &zero, &zero, &zero, &zero);
}
bool nECU_testUserSettings(void) // test both read and write to flash memory
{
    bool d0, d1;
    d0 = true;
    d1 = false;
    nECU_saveUserSettings(&d0, &d1);
    nECU_readUserSettings(&d0, &d1);
    if (d0 != true || d1 != false)
    {
        return false;
    }

    d0 = false;
    d1 = true;
    nECU_saveUserSettings(&d0, &d1);
    nECU_readUserSettings(&d0, &d1);
    if (d0 != false || d1 != true)
    {
        return false;
    }

    return true;
}

/* Flash functions */
void nECU_FLASH_cleanFlashSector(void) // clean flash sector
{
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_11, FLASH_VOLTAGE_RANGE_3);
    FLASH_FlushCaches();
    HAL_FLASH_Lock();
}
void nECU_FLASH_getAllMemory(void) // get data from flash
{
    nECU_FLASH_readSpeedCalibrationData(&(Flash.speedData)); // get speed calib to RAM
    nECU_FLASH_readUserSettings(&(Flash.userData));          // get user settings to RAM
    if (Flash.dataInitialized == false)
    {
        Flash.dataInitialized = true;
    }
}
void nECU_FLASH_saveFlashSector(void) // save everything, then read to RAM
{
    nECU_FLASH_cleanFlashSector();                            // clear all data
    nECU_FLASH_writeSpeedCalibrationData(&(Flash.speedData)); // save speed calib
    nECU_FLASH_writeUserSettings(&(Flash.userData));          // save user settings
    nECU_FLASH_getAllMemory();                                // update RAM
}
void nECU_FLASH_writeSpeedCalibrationData(const SpeedCalibrationData *data) // function to write calibration data to flash memory
{
    if (memcmp(&(Flash.speedData), data, sizeof(SpeedCalibrationData))) // check if memory needs to be updated
    {
        Flash.speedData = *data; // copy new data to buffer
        HAL_FLASH_Unlock();
        FLASH_FlushCaches();

        // Write the data to flash memory
        for (int i = 0; i < sizeof(SpeedCalibrationData); i += 4)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + SPEED_DATA_OFFSET + i, *(uint32_t *)((uint8_t *)data + i));
        }

        HAL_FLASH_Lock();
    }
}
void nECU_FLASH_readSpeedCalibrationData(SpeedCalibrationData *data) // function to read calibration data from flash memory
{
    // Copy the data from flash memory to the data structure
    memcpy(data, (const void *)FLASH_DATA_START_ADDRESS + SPEED_DATA_OFFSET, sizeof(SpeedCalibrationData));

    // check if proper data was read
    if (data->SpeedSensor1 == 0x7FFFFFFF)
    {
        data->SpeedSensor1 = 0.0;
    }
    if (data->SpeedSensor2 == 0x7FFFFFFF)
    {
        data->SpeedSensor2 = 0.0;
    }
    if (data->SpeedSensor3 == 0x7FFFFFFF)
    {
        data->SpeedSensor3 = 0.0;
    }
    if (data->SpeedSensor4 == 0x7FFFFFFF)
    {
        data->SpeedSensor4 = 0.0;
    }
}
void nECU_FLASH_writeUserSettings(const UserSettings *data) // function to write settings data to flash memory
{
    if (memcmp(&(Flash.userData), data, sizeof(UserSettings))) // check if memory needs to be updated
    {
        Flash.userData = *data; // copy new data to buffer
        HAL_FLASH_Unlock();
        FLASH_FlushCaches();

        // Write the data to flash memory
        for (int i = 0; i < sizeof(UserSettings); i += 1)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + USER_SETTINGS_OFFSET + i, *(uint8_t *)((uint8_t *)data + i));
        }

        HAL_FLASH_Lock();
    }
}
void nECU_FLASH_readUserSettings(UserSettings *data) // function to read settings data to flash memory
{
    // Copy the data from flash memory to the data structure
    memcpy(data, (const void *)FLASH_DATA_START_ADDRESS + USER_SETTINGS_OFFSET, sizeof(UserSettings));

    // check if proper data was read
    if (data->boolByte1 == 0xFF)
    {
        data->boolByte1 = 0;
    }
}

/* Helper functions */
void nECU_compressBool(bool *d0, bool *d1, bool *d2, bool *d3, bool *d4, bool *d5, bool *d6, bool *d7, uint8_t *out) // compress bools to one byte
{
    // create temporary buffer, fill with input data
    bool data[8] = {*d0, *d1, *d2, *d3, *d4, *d5, *d6, *d7};

    // zero-out output
    *out = 0;

    // fill output with data
    for (uint8_t i = 0; i < 8; i++)
    {
        *out |= (data[i] & 1) >> i;
    }
}
void nECU_decompressBool(uint8_t *in, bool *d0, bool *d1, bool *d2, bool *d3, bool *d4, bool *d5, bool *d6, bool *d7) // decompress byte to bools
{
    // create temporary buffer
    bool data[8];

    // fill buffer with data
    for (uint8_t i = 0; i < 8; i++)
    {
        data[i] = ((*in) << i) & 1;
    }

    // copy to outputs
    *d0 = data[0];
    *d1 = data[1];
    *d2 = data[2];
    *d3 = data[3];
    *d4 = data[4];
    *d5 = data[5];
    *d6 = data[6];
    *d7 = data[7];
}