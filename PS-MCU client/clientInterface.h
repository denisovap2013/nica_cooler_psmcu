//==============================================================================
//
// Title:       clientInterface.h
// Purpose:     A short description of the interface.
//
// Created on:  28.09.2015 at 13:08:35 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

#ifndef __clientInterface_H__
#define __clientInterface_H__

//==============================================================================
// Include files

#include "cvidef.h"
#include "clientData.h"

//==============================================================================
// Constants
#define BLOCK_BUTTON_HEIGHT 20 
#define BLOCK_BUTTON_WIDTH  160
#define BLOCK_ELEMENT_STEP 20
#define ADC_ELEMENTS_STEP 2
#define ADC_BUTTON_HEIGHT 20
#define ADC_BUTTON_WIDTH 20
#define DAC_BUTTON_HEIGHT 20
#define DAC_BUTTON_WIDTH 30 
#define ADC_FIELD_WIDTH 70
#define DAC_FIELD_WIDTH 70 
#define DAC_CONF_FIELD_WIDTH 50
#define CMD_BTN_WIDTH 60
#define CMD_BTN_WIDTH_2 80 
#define CMD_BTN_MARGIN 2 

#define TIMER_TICK_TIME 0.100 


// Main menu block
extern int mainMenuHandle;
extern int psMcuWindowHandles[PSMCU_MAX_NUM]; 

extern int SERVER_CONNECTION_INDICATOR;
extern int CANGW_CONNECTION_INDICATOR;

extern int PSMCU_WINDOW_BUTTONS[PSMCU_MAX_NUM];
extern int PSMCU_STATUS_INDICATOR[PSMCU_MAX_NUM];
extern int PSMCU_WINDOW_FIELDS_CAPTION; 
extern int PSMCU_ORDER_LABELS[PSMCU_MAX_NUM];
extern int PSMCU_PIPELINE_STOP_BTN;

// Specific Interface elements for each power supply
extern int PSMCU_ADC_GRAPH_BUTTONS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];

extern int PSMCU_BLOCK_ADC_FIELDS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM]; 
extern int PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[PSMCU_MAX_NUM];  
extern int PSMCU_BLOCK_DAC_FIELDS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM];
extern int PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[PSMCU_MAX_NUM]; 
extern int PSMCU_BLOCK_DAC_CONF_FIELDS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM];

extern int PSMCU_BLOCK_INREG_INDICATORS[PSMCU_MAX_NUM][PSMCU_INREG_BITS_NUM]; 
extern int PSMCU_BLOCK_INREG_DUPLICATE_INDICATOR[PSMCU_MAX_NUM];  
extern int PSMCU_BLOCK_OUTREG_INDICATORS[PSMCU_MAX_NUM][PSMCU_OUTREG_BITS_NUM];
extern int PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[PSMCU_MAX_NUM][2]; 
extern int PSMCU_BLOCK_INTERLOCK_RESET_BTNS[PSMCU_MAX_NUM]; 
extern int PSMCU_BLOCK_CURRENT_PERM_BTNS[PSMCU_MAX_NUM][2]; 
extern int PSMCU_BLOCK_FORCE_BTNS[PSMCU_MAX_NUM][2];
extern int PSMCU_BLOCK_ZERO_FAST_BTN[PSMCU_MAX_NUM];

extern int PSMCU_GRAPHS_WINDOW_HANDLES[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];

//==============================================================================
// Global functions
		
void initValues(void);
void initGui(void); 
void setupSinglePsMcuGui(int);
void DiscardGuiResources(void);

void UpdateGraphs(void);    
void resolveCommandButtonStatus(int deviceIndex, unsigned int inputRegisters, unsigned int outputRegisters, double dacVoltage);
void resolvePsmcuColorIndicators(
	int deviceIndex,
	unsigned int inputRegisters,
	double dacSentVoltage,
	double dacReadVoltage,
	double adcVoltage,
	double dac_dac_max_diff,
	double dac_adc_max_diff
);

void UpdateDeviceName(int deviceIndex, char *name);
void UpdateGraphTitle(int deviceIndex, int channelIndex);

int CVICALLBACK mainMenuButtonCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK adcButtonCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK dacFieldCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int clientCallbackFunction(unsigned handle, int xType, int errCode, void * callbackData);

int CVICALLBACK mainSystemCallback (int handle, int control, int event, void *callbackData, 
									int eventData1, int eventData2);


int CVICALLBACK interlockResetCmdBtnsCallback (int handle, int control, int event, void *callbackData, 
									           int eventData1, int eventData2);

int CVICALLBACK currentOnCmdBtnsCallback (int handle, int control, int event, void *callbackData, 
									           int eventData1, int eventData2);

int CVICALLBACK currentOffCmdBtnsCallback (int handle, int control, int event, void *callbackData, 
									           int eventData1, int eventData2);

int CVICALLBACK forceOnCmdBtnsCallback (int handle, int control, int event, void *callbackData, 
									           int eventData1, int eventData2);

int CVICALLBACK forceOffCmdBtnsCallback (int handle, int control, int event, void *callbackData, 
									           int eventData1, int eventData2);

int CVICALLBACK zeroSingleFastBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK loadDacBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK saveDacBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK zeroAllFastBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK zeroAllSmartBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK allResetBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK allForceOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK allForceOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK allCurrentOnBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK allCurrentOffBtnCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);

int CVICALLBACK stopPipelineCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2);


#endif  /* ndef __clientInterface_H__ */
