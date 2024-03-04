/**
 ******************************************************************************
 * @file    nECU_flash.c
 * @brief   This file provides code for user defined flash access functions.
 ******************************************************************************
 */

#include "nECU_flash.h"

nECU_FlashContent Flash;
bool FlashdataInitialized = false;

/* Speed calibration data functions (flash function interface) */
void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy config if needed to buffer
    Flash.speedData.SpeedSensor1 = *Sensor1;
    Flash.speedData.SpeedSensor2 = *Sensor2;
    Flash.speedData.SpeedSensor3 = *Sensor3;
    Flash.speedData.SpeedSensor4 = *Sensor4;

    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy data to outputs
    *Sensor1 = Flash.speedData.SpeedSensor1;
    *Sensor2 = Flash.speedData.SpeedSensor2;
    *Sensor3 = Flash.speedData.SpeedSensor3;
    *Sensor4 = Flash.speedData.SpeedSensor4;
}

/* User settings data functions (flash function interface) */
void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // compress data to byte
    bool compressed[8] = {*pAntiLag, *pTractionOFF, 0, 0, 0, 0, 0, 0};
    nECU_compressBool(compressed, &(Flash.userData.boolByte1));
    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // decompress to output
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
}

/* Debug que (flash function interface) */
/* Que not stored in RAM as it is a big structure */
void nECU_saveDebugQue(nECU_Debug_error_que *que)
{
    nECU_FLASH_writeDebugQue(que);
}
void nECU_readDebugQue(nECU_Debug_error_que *que)
{
    nECU_FLASH_readDebugQue(que);
}

/* Flash functions */
void nECU_FLASH_cleanFlashSector(void) // clean flash sector
{
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_11, FLASH_VOLTAGE_RANGE_3);
    FLASH_FlushCaches();
    HAL_FLASH_Lock();
}
void nECU_FLASH_cleanFlashSector_check(void) // check if erase was successful
{
    /* Check every byte of avaliable space */
    for (uint16_t i = 0; i < (FLASH_DATA_END_ADDRESS - FLASH_DATA_START_ADDRESS); i++)
    {
        if (memcmp((const void *)UINT8_MAX, (const void *)FLASH_DATA_START_ADDRESS + i, sizeof(uint8_t)))
        {
            nECU_Debug_FLASH_error(nECU_FLASH_ERROR_ERASE, false);
        }
    }
}
void nECU_FLASH_getAllMemory(void) // get data from flash
{
    nECU_FLASH_readSpeedCalibrationData(&(Flash.speedData)); // get speed calib to RAM
    nECU_FLASH_readUserSettings(&(Flash.userData));          // get user settings to RAM
    if (FlashdataInitialized == false)
    {
        FlashdataInitialized = true;
    }
}
void nECU_FLASH_saveFlashSector(void) // save everything, then read to RAM
{
    nECU_FLASH_writeSpeedCalibrationData(&(Flash.speedData)); // save speed calib
    nECU_FLASH_writeUserSettings(&(Flash.userData));          // save user settings
    nECU_FLASH_getAllMemory();                                // update RAM
}

/* Dedicated to data type functions */
void nECU_FLASH_writeSpeedCalibrationData(nECU_SpeedCalibrationData *data) // function to write calibration data to flash memory
{
    Flash.speedData = *data; // copy new data to buffer
    HAL_FLASH_Unlock();
    FLASH_FlushCaches();

    // Write the data to flash memory
    for (int i = 0; i < sizeof(nECU_SpeedCalibrationData); i += 4)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + SPEED_DATA_OFFSET + i, *(uint32_t *)((uint8_t *)data + i));
    }
    HAL_FLASH_Lock();
    nECU_FLASH_checkSpeedCalibrationData(data);
}
void nECU_FLASH_readSpeedCalibrationData(nECU_SpeedCalibrationData *data) // function to read calibration data from flash memory
{
    memset(data, UINT8_MAX, sizeof(nECU_SpeedCalibrationData)); // fil target with FF
    // Copy the data from flash memory to the data structure
    memcpy(data, (const void *)FLASH_DATA_START_ADDRESS + SPEED_DATA_OFFSET, sizeof(nECU_SpeedCalibrationData));

    // check if proper data was read
    if (data->SpeedSensor1 == UINT32_MAX)
    {
        data->SpeedSensor1 = 0.0;
    }
    if (data->SpeedSensor2 == UINT32_MAX)
    {
        data->SpeedSensor2 = 0.0;
    }
    if (data->SpeedSensor3 == UINT32_MAX)
    {
        data->SpeedSensor3 = 0.0;
    }
    if (data->SpeedSensor4 == UINT32_MAX)
    {
        data->SpeedSensor4 = 0.0;
    }
    nECU_FLASH_checkSpeedCalibrationData(data);
}
void nECU_FLASH_checkSpeedCalibrationData(nECU_SpeedCalibrationData *data) // check if passed data is the same as in memory
{
    int comparison_result = 0;
    comparison_result = memcmp(data, (const void *)FLASH_DATA_START_ADDRESS + DEBUG_QUE_OFFSET, sizeof(nECU_SpeedCalibrationData));
    if (comparison_result < 0) // saving
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, true);
    }
    else if (comparison_result > 0) // reading
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, false);
    }
}

void nECU_FLASH_writeUserSettings(nECU_UserSettings *data) // function to write settings data to flash memory
{
    Flash.userData = *data; // copy new data to buffer
    HAL_FLASH_Unlock();
    FLASH_FlushCaches();

    // Write the data to flash memory
    for (int i = 0; i < sizeof(nECU_UserSettings); i += 1)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + USER_SETTINGS_OFFSET + i, *(uint8_t *)((uint8_t *)data + i));
    }

    HAL_FLASH_Lock();
    nECU_FLASH_checkUserSettings(data);
}
void nECU_FLASH_readUserSettings(nECU_UserSettings *data) // function to read settings data to flash memory
{
    memset(data, UINT8_MAX, sizeof(nECU_UserSettings)); // fil target with FF
    // Copy the data from flash memory to the data structure
    memcpy(data, (const void *)FLASH_DATA_START_ADDRESS + USER_SETTINGS_OFFSET, sizeof(nECU_UserSettings));

    // check if proper data was read
    if (data->boolByte1 == UINT8_MAX)
    {
        data->boolByte1 = 0;
    }
    nECU_FLASH_checkUserSettings(data);
}
void nECU_FLASH_checkUserSettings(nECU_UserSettings *data) // check if passed data is the same as in memory
{
    int comparison_result = 0;
    comparison_result = memcmp(data, (const void *)FLASH_DATA_START_ADDRESS + DEBUG_QUE_OFFSET, sizeof(nECU_UserSettings));
    if (comparison_result < 0) // saving
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, true);
    }
    else if (comparison_result > 0) // reading
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, false);
    }
}

void nECU_FLASH_writeDebugQue(nECU_Debug_error_que *que) // function to write debug que to flash memory
{
    HAL_FLASH_Unlock();
    FLASH_FlushCaches();

    // Write the data to flash memory
    for (int i = 0; i < sizeof(nECU_Debug_error_que); i += 1)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + DEBUG_QUE_OFFSET + i, *(uint8_t *)((uint8_t *)que + i));
    }

    HAL_FLASH_Lock();
    nECU_FLASH_checkDebugQue(que);
}
void nECU_FLASH_readDebugQue(nECU_Debug_error_que *que) // function to read debug que to flash memory
{
    memset(que, UINT8_MAX, sizeof(nECU_Debug_error_que));                                                 // fil target with FF
    memcpy(que, (const void *)FLASH_DATA_START_ADDRESS + DEBUG_QUE_OFFSET, sizeof(nECU_Debug_error_que)); // copy the que

    if (que->message_count == UINT16_MAX) // if que is maximum (no memory was written beforehand) set messages to zero
    {
        memset(que, 0, sizeof(nECU_Debug_error_que)); // zero-out memory region
        que->message_count = 0;
    }
    nECU_FLASH_checkDebugQue(que);
}
void nECU_FLASH_checkDebugQue(nECU_Debug_error_que *que) // check if passed que is the same as in memory
{
    int comparison_result = 0;
    comparison_result = memcmp(que, (const void *)FLASH_DATA_START_ADDRESS + DEBUG_QUE_OFFSET, sizeof(nECU_Debug_error_que));
    if (comparison_result < 0) // saving
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, true);
    }
    else if (comparison_result > 0) // reading
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, false);
    }
}

/* Helper functions */
void nECU_compressBool(bool *bufferIn, uint8_t *out) // compress bool array to one byte
{
    // zero-out output
    *out = 0;

    // fill output with data
    for (uint8_t i = 0; i < 8; i++)
    {
        *out |= (bufferIn[i] & 1) >> i;
    }
}
void nECU_decompressBool(uint8_t *in, bool *bufferOut) // decompress byte to bool array
{
    for (uint8_t i = 0; i < 8; i++)
    {
        bufferOut[i] = ((*in) << i) & 1; // copy to outputs
    }
}
