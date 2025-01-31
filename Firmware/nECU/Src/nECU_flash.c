/**
 ******************************************************************************
 * @file    nECU_flash.c
 * @brief   This file provides code for user defined flash access functions.
 ******************************************************************************
 */

#include "nECU_flash.h"

static nECU_FlashContent Flash = {0};

/* Speed calibration data functions (flash function interface) */
bool nECU_Flash_SpeedCalibration_save(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    bool status = false;

    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }

    float tempSensor[] = {*Sensor1, *Sensor2, *Sensor3, *Sensor4};

    if (memcmp(&(Flash.speedData), &tempSensor, sizeof(tempSensor))) // break if they are the same
    {
        return status;
    }

    // copy config if needed to buffer
    Flash.speedData.SpeedSensor1 = *Sensor1;
    Flash.speedData.SpeedSensor2 = *Sensor2;
    Flash.speedData.SpeedSensor3 = *Sensor3;
    Flash.speedData.SpeedSensor4 = *Sensor4;

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_Flash_SpeedCalibration_read(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    bool status = false;

    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }

    // copy data to outputs
    *Sensor1 = Flash.speedData.SpeedSensor1;
    *Sensor2 = Flash.speedData.SpeedSensor2;
    *Sensor3 = Flash.speedData.SpeedSensor3;
    *Sensor4 = Flash.speedData.SpeedSensor4;

    nECU_Debug_ProgramBlockData_Update(D_Flash);

    return status;
}

/* User settings data functions (flash function interface) */
bool nECU_Flash_UserSettings_save(bool *pAntiLag, bool *pTractionOFF)
{
    bool status = false;

    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }

    // prepare buffers for comparison
    bool compressed[8] = {*pAntiLag, *pTractionOFF, 0, 0, 0, 0, 0, 0};
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
    if (memcmp(compressed, decompressed, sizeof(compressed))) // break if they are the same
    {
        return status;
    }

    // compress data to byte
    nECU_compressBool(compressed, &(Flash.userData.boolByte1));

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_Flash_UserSettings_read(bool *pAntiLag, bool *pTractionOFF)
{
    bool status = false;

    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }

    // decompress to output
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
    *pAntiLag = decompressed[0];
    *pTractionOFF = decompressed[1];

    nECU_Debug_ProgramBlockData_Update(D_Flash);

    return status;
}

/* Debug que (flash function interface) */
bool nECU_Flash_DebugQue_save(nECU_Debug_error_que *que)
{
    bool status = false;

    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }

    // Set pointer to array -> debug que is so long it is pointless to store it in RAM
    Flash.DebugQueData = que;

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_Flash_DebugQue_read(nECU_Debug_error_que *que)
{
    // This block does not require whole flash module to have status "Working"
    bool status = false;
    Flash.DebugQueData = que;

    memcpy(Flash.DebugQueData, (const void *)FLASH_DATA_START_ADDR_DEBUGQUE, sizeof(nECU_Debug_error_que)); // user settings to RAM

    if (memcmp(Flash.DebugQueData, (const void *)FLASH_DATA_START_ADDR_DEBUGQUE, sizeof(nECU_Debug_error_que))) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, false);
        status |= true;
    }

    return status;
}

/* Flash functions */
static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector(void) // clean flash sector
{
    HAL_StatusTypeDef status = HAL_OK;

    status |= HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_11, FLASH_VOLTAGE_RANGE_3);
    FLASH_FlushCaches();
    status |= HAL_FLASH_Lock();
    status |= nECU_FLASH_cleanFlashSector_check();

    if (status != HAL_OK)
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_ERASE, true);
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector_check(void) // check if erase was successful
{
    HAL_StatusTypeDef status = HAL_OK;

    uint8_t empty_array[(FLASH_DATA_END_ADDRESS - FLASH_DATA_START_ADDRESS)] = {0};
    if (memcmp(empty_array, (const void *)FLASH_DATA_START_ADDRESS, (FLASH_DATA_END_ADDRESS - FLASH_DATA_START_ADDRESS)))
    {
        status = HAL_ERROR;
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_getAllMemory(void) // get data from flash
{
    HAL_StatusTypeDef status = HAL_OK;

    /* copy data to the RAM */
    memcpy(&(Flash.speedData), (const void *)FLASH_DATA_START_ADDRESS, sizeof(nECU_SpeedCalibrationData)); // speed data to RAM
    memcpy(&(Flash.userData), (const void *)FLASH_DATA_START_ADDR_USER, sizeof(nECU_UserSettings));        // user settings to RAM

    /* Check if reading was successful */
    if (memcmp(&(Flash.speedData), (const void *)FLASH_DATA_START_ADDRESS, sizeof(nECU_SpeedCalibrationData)) != 0) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, false);
        status |= HAL_ERROR;
    }
    if (memcmp(&(Flash.userData), (const void *)FLASH_DATA_START_ADDR_USER, sizeof(nECU_UserSettings)) != 0) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, false);
        status |= HAL_ERROR;
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_saveFlashSector(void) // save everything, then read to RAM
{
    HAL_StatusTypeDef status = HAL_OK;

    status |= nECU_FLASH_cleanFlashSector();                                             // prepare memory for a save
    uint16_t byte_count = sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings); // define buffer length

    if (nECU_FlowControl_Initialize_Check(D_Debug_Que))
    {
        byte_count += sizeof(nECU_Debug_error_que); // add debug error que if it was initialized
    }

    uint8_t data[byte_count]; // create buffer (it has to be devidable by 4 so be carefull)

    /* copy data to the buffer */
    memcpy(&data[0], &(Flash.speedData), sizeof(nECU_SpeedCalibrationData));                        // copy speed data
    memcpy(&data[sizeof(nECU_SpeedCalibrationData)], &(Flash.userData), sizeof(nECU_UserSettings)); // copy user settings data
    if (nECU_FlowControl_Initialize_Check(D_Debug_Que))                                             // copy debug que if it was initialized
    {
        memcpy(&data[sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings)], Flash.DebugQueData, sizeof(nECU_Debug_error_que));
    }

    /* Write the data to flash memory */
    status |= HAL_FLASH_Unlock();
    for (int i = 0; i < byte_count; i += 4)
    {
        status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + i, *(uint32_t *)((uint8_t *)data + i));
    }
    status |= HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, true);
    }

    status |= nECU_FLASH_getAllMemory(); // update RAM

    if (memcmp(&data[0], (const void *)FLASH_DATA_START_ADDRESS, byte_count) != 0) // check if save was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, true);
    }

    nECU_Debug_ProgramBlockData_Update(D_Flash);

    return status;
}

/* Interface functions */
bool nECU_FLASH_Start(void) // initialize FLASH code
{
    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_Flash))
    {
        status |= (nECU_FLASH_getAllMemory() != HAL_OK);
        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Check(D_Flash);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Check(D_Flash);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Flash);
    }

    return status;
}
bool nECU_FLASH_Erase(void) // erases whole sector
{
    bool status = false;
    // check if data was initialized
    if (!nECU_FlowControl_Working_Check(D_Flash))
    {
        nECU_FlowControl_Error_Do(D_Flash);
        status |= true;
        return status;
    }
    status |= (nECU_FLASH_cleanFlashSector() != HAL_OK);
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Flash);
    }

    return status;
}
