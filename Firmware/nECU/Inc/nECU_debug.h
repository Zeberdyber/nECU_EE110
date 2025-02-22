/**
 ******************************************************************************
 * @file    nECU_debug.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_debug.c file
 */
#ifndef nECU_debug_H_
#define nECU_debug_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "nECU_debug.h"
#include "nECU_types.h"
#include "nECU_can.h"
#include "nECU_spi.h"

/* Definitions */
#define DEVICE_TEMPERATURE_MAX 55  // in deg C
#define DEVICE_TEMPERATURE_MIN -20 // in deg C
#define TC_TEMPERATURE_MAX 1000    // in deg C
#define BENCH_MODE true            // set true if not connected to car harness

    /* Debug main functions */
    bool nECU_Debug_Start(void);              // starts up debugging functions
    static bool nECU_Debug_Init_Struct(void); // set values to variables in structure
    void nECU_Debug_Periodic(void);           // checks states of variables

    /* Check states routines */
    static void nECU_Debug_IntTemp_Check(nECU_Debug_IC_temp *inst);    // check for errors of device temperature
    static bool nECU_Debug_IntTemp_CheckSingle(int16_t *temperature);  // checks if passed temperature is in defined bounds
    static void nECU_Debug_EGTTemp_Check(nECU_Debug_EGT_Temp *inst);   // check if TCs did not exceed fault value
    static bool nECU_Debug_EGTTemp_CheckSingle(uint16_t *temperature); // checks if passed temperature is in defined bound
    static void nECU_Debug_EGTsensor_error(nECU_Debug_EGT_Comm *inst); // to be called when TC sensor error occurs
    static void nECU_Debug_CAN_Check(void);                            // checks if CAN have any error pending
    static void nECU_Debug_SPI_Check(void);                            // checks if SPI have any error pending

    /* Functions to call directly from other files */
    void nECU_Debug_EGTSPIcomm_error(EGT_Sensor_ID ID);                   // to be called when SPI error occurs
    void nECU_Debug_FLASH_error(nECU_Flash_Error_ID ID, bool write_read); // indicate error from flash functions

    /* Debug que and messages */
    static bool nECU_Debug_Init_Que(void);                                                     // initializes que
    static void nECU_Debug_Que_Write(nECU_Debug_error_mesage *message);                        // add message to debug que
    void nECU_Debug_Que_Read(nECU_Debug_error_mesage *message);                                // read newest message from debug que
    static void nECU_Debug_Message_Init(nECU_Debug_error_mesage *inst);                        // zeros value inside of structure
    void nECU_Debug_Message_Set(nECU_Debug_error_mesage *inst, float value, nECU_Error_ID ID); // sets error values

#ifdef __cplusplus
}
#endif

#endif /* _nECU_debug_H__ */