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
    void nECU_Debug_ProgramBlock_Init(void);                                                // Initialize 'ProgramBlock' tracking
    static void nECU_Debug_ProgramBlockData_Clear(nECU_ProgramBlockData *inst);             // Clear structure 'ProgramBlockData'
    void nECU_Debug_ProgramBlockData_Update(nECU_Module_ID ID);                             // Update tick tracking and check for timeout
    void nECU_Debug_ProgramBlockData_Check(void);                                           // Perform error check for all blocks
    static uint8_t nECU_Debug_ProgramBlockData_Check_Single(nECU_ProgramBlockData *inst);   // returns if errors occur
    nECU_ProgramBlockData *nECU_Debug_ProgramBlockData_getPointer_Block(nECU_Module_ID ID); // returns pointer to given ID program block
    uint32_t *nECU_Debug_ProgramBlockData_getPointer_Diff(nECU_Module_ID ID);               // returns pointer to time difference

    /* Flow control */
    bool nECU_FlowControl_Stop_Check(nECU_Module_ID ID);        // Check if block has "initialized" status
    bool nECU_FlowControl_Stop_Do(nECU_Module_ID ID);           // Write "initialized" status if possible
    bool nECU_FlowControl_Initialize_Check(nECU_Module_ID ID);  // Check if block has "initialized" status
    bool nECU_FlowControl_Initialize_Do(nECU_Module_ID ID);     // Write "initialized" status if possible
    bool nECU_FlowControl_Working_Check(nECU_Module_ID ID);     // Check if block has "working" status
    bool nECU_FlowControl_Working_Do(nECU_Module_ID ID);        // Write "working" status if possible
    bool nECU_FlowControl_Error_Check(nECU_Module_ID ID);       // Check if block has "error" status
    bool nECU_FlowControl_Error_Do(nECU_Module_ID ID);          // Write "error" status if possible
    bool nECU_FlowControl_DoubleError_Check(nECU_Module_ID ID); // Check if block has "error_old" status
    bool nECU_FlowControl_DoubleError_Do(nECU_Module_ID ID);    // Write "error_old" status if possible
#ifdef __cplusplus
}
#endif

#endif /* _nECU_flowControl_H__ */