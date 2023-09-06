//==============================================================================
//
// Title:       clientData.h
// Purpose:     A short description of the interface.
//
// Created on:  09.12.2021 at 12:23:41 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __clientData_H__
#define __clientData_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"

//==============================================================================
// Constants
#define PSMCU_ADC_CHANNELS_NUM 5
#define PSMCU_DAC_CHANNELS_NUM 1 
#define PSMCU_INREG_BITS_NUM 4
#define PSMCU_OUTREG_BITS_NUM 3
#define PSMCU_MAX_NUM 20
//==============================================================================
// Types

//==============================================================================
// External variables
extern int PSMCU_NUM;
		
extern double ADC_STORED_VALS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];
extern double DAC_CLIENT_SENT_VALUES[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM]; 
extern double DAC_SERVER_READ_VALS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM]; 
extern unsigned int STORED_INPUT_REGS[PSMCU_MAX_NUM];
extern unsigned int STORED_OUTPUT_REGS[PSMCU_MAX_NUM];
extern unsigned int PSMCU_ALIVE_STATUS[PSMCU_MAX_NUM];

extern char PSMCU_ADC_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM][256];
extern char PSMCU_DAC_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM][256];
extern char PSMCU_INREG_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_INREG_BITS_NUM][256];
extern char PSMCU_OUTREG_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_OUTREG_BITS_NUM][256];   
extern char PSMCU_DEV_TITLE[PSMCU_MAX_NUM][256]; 
extern char PSMCU_DEV_NAME[PSMCU_MAX_NUM][256];

extern char SERVER_INDICATOR_ONLINE_LABEL[256];
extern char SERVER_INDICATOR_OFFLINE_LABEL[256];
extern char CANGW_INDICATOR_ONLINE_LABEL[256];
extern char CANGW_INDICATOR_OFFLINE_LABEL[256];

extern int PSMCU_DEVICES_ORDER[PSMCU_MAX_NUM];
extern int PSMCU_DEVICES_PRIORITY[PSMCU_MAX_NUM];
//==============================================================================
// Global functions
void clearNames(void); 
int initDevicesOrder(int *order, char *errMsgBuf);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __clientData_H__ */
