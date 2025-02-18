/**
 ******************************************************************************
 * @file    nECU_flowControl.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_flowControl.c file
 */
#ifndef nECU_flowControl_H_
#define nECU_flowControl_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdio.h"
#include "stdbool.h"
#include "nECU_types.h"
#include "nECU_tim.h"

    /* Definitions */
#define PROGRAMBLOCK_TIMEOUT_DEFAULT 5 // number of seconds that will cause a timeout

    /* Program Block */
    void nECU_FC_Start(void); // Initialize 'ProgramBlock' tracking

    /* Flow control */
    static bool nECU_FC_Init_Check(nECU_Module_ID ID);               // check if was structure initialized
    static bool nECU_FC_Init_Do(nECU_Module_ID ID, uint8_t timeout); // Initialize structure

    bool nECU_FC_Stop_Check(nECU_Module_ID ID); // Check if block has "stop" status
    bool nECU_FC_Stop_Do(nECU_Module_ID ID);    // Write "stop" status if possible

    bool nECU_FC_Initialize_Check(nECU_Module_ID ID); // Check if block has "initialized" status
    bool nECU_FC_Initialize_Do(nECU_Module_ID ID);    // Write "initialized" status if possible

    bool nECU_FC_Working_Check(nECU_Module_ID ID);       // Check if block has "working" status
    bool nECU_FlowControl_Working_Do(nECU_Module_ID ID); // Write "working" status if possible

    bool nECU_FC_Error_Check(nECU_Module_ID ID); // Check if block has "error" status
    bool nECU_FC_Error_Do(nECU_Module_ID ID);    // Write "error" status if possible

    static bool nECU_FC_DoubleError_Check(nECU_Module_ID ID); // Check if block has "error_old" status
    static bool nECU_FC_DoubleError_Do(nECU_Module_ID ID);    // Write "error_old" status if possible

    bool nECU_FC_Timeout_Check(nECU_Module_ID ID);        // check if timeout occured
    static bool nECU_FC_Timeout_Do(nECU_Module_ID ID);    // Perform action for timeout
    uint32_t nECU_FC_Timeout_getValue(nECU_Module_ID ID); // Returns value of difference

    static bool nECU_FC_WriteMessage(nECU_Module_ID ID); // Write message to debug que

#ifdef __cplusplus
}
#endif

#endif /* _nECU_flowControl_H__ */