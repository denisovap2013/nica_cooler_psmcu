//==============================================================================
//
// Title:       clientData.c
// Purpose:     A short description of the implementation.
//
// Created on:  09.12.2021 at 12:23:41 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <ansi_c.h>
#include "clientData.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions

//==============================================================================
// Global variables
int PSMCU_NUM = 1; 

double ADC_STORED_VALS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];
double DAC_CLIENT_SENT_VALUES[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM];
double DAC_SERVER_READ_VALS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM]; 
unsigned int STORED_INPUT_REGS[PSMCU_MAX_NUM];
unsigned int STORED_OUTPUT_REGS[PSMCU_MAX_NUM];
unsigned int PSMCU_ALIVE_STATUS[PSMCU_MAX_NUM]; 

char PSMCU_ADC_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM][256];
char PSMCU_DAC_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM][256];
char PSMCU_INREG_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_INREG_BITS_NUM][256];
char PSMCU_OUTREG_LABELS_TEXT[PSMCU_MAX_NUM][PSMCU_OUTREG_BITS_NUM][256];
char PSMCU_DEV_TITLE[PSMCU_MAX_NUM][256];
char PSMCU_DEV_NAME[PSMCU_MAX_NUM][256];

char SERVER_INDICATOR_ONLINE_LABEL[256];
char SERVER_INDICATOR_OFFLINE_LABEL[256];
char CANGW_INDICATOR_ONLINE_LABEL[256];
char CANGW_INDICATOR_OFFLINE_LABEL[256];

int PSMCU_DEVICES_ORDER[PSMCU_MAX_NUM];
int PSMCU_DEVICES_PRIORITY[PSMCU_MAX_NUM];
//==============================================================================
// Global functions


void clearNames(void) {
	int i,j;
	// devices names
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		strcpy(PSMCU_DEV_NAME[i], "Unknown");	
		sprintf(PSMCU_DEV_TITLE[i], "%d: %s", i+1, PSMCU_DEV_NAME[i]);
	}
	
	// ADCs 
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		for (j=0;j < PSMCU_ADC_CHANNELS_NUM; j++) {
			strcpy(PSMCU_ADC_LABELS_TEXT[i][j], ""); 			
		}
	}
	
	// DACs 
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		for (j=0;j < PSMCU_DAC_CHANNELS_NUM; j++) {
			strcpy(PSMCU_DAC_LABELS_TEXT[i][j], ""); 			
		}
	}
	
	// Input registers
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		for (j=0;j < PSMCU_INREG_BITS_NUM; j++) {
			strcpy(PSMCU_INREG_LABELS_TEXT[i][j], ""); 			
		}
	}
	
	// Output registers
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		for (j=0;j < PSMCU_OUTREG_BITS_NUM; j++) {
			strcpy(PSMCU_OUTREG_LABELS_TEXT[i][j], ""); 			
		}
	}
	
	// CANGW connection
	strcpy(CANGW_INDICATOR_ONLINE_LABEL, "CanGw is Online");
	strcpy(CANGW_INDICATOR_OFFLINE_LABEL, "CanGw is Offline"); 
}


int initDevicesOrder(int *order, char *errMsgBuf) {
	// 0 - OK, -1 - something's wrong

	int orderIndex, deviceIndex;
	
	for (deviceIndex=0; deviceIndex < PSMCU_MAX_NUM; deviceIndex++) {
		PSMCU_DEVICES_PRIORITY[deviceIndex] = -1;		
	}
	
	for (orderIndex=0; orderIndex < PSMCU_NUM; orderIndex++) {
		deviceIndex = order[orderIndex];
		
		if (deviceIndex < 0 || deviceIndex >= PSMCU_NUM) {
			if (errMsgBuf != NULL) sprintf(errMsgBuf, "For the %d-th element of the specified order the device index %d is out of range [0, %d].", orderIndex, deviceIndex, PSMCU_NUM-1);
			return -1;
		}
		
		if (PSMCU_DEVICES_PRIORITY[deviceIndex] >= 0) {
			if (errMsgBuf != NULL) sprintf(errMsgBuf, "For the %d-th element of the specified order the device index %d occurs twice.", orderIndex, deviceIndex); 
			return -1;
		}
		
		PSMCU_DEVICES_ORDER[orderIndex] = deviceIndex;
		PSMCU_DEVICES_PRIORITY[deviceIndex] = orderIndex;
	}
	
	return 0;
}
