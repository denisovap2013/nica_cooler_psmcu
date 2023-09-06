//==============================================================================
//
// Title:       clientInterface.c
// Purpose:     A short description of the implementation.
//
// Created on:  28.09.2015 at 13:08:35 by OEM.
// Copyright:   OEM. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <userint.h>
#include <ansi_c.h>
#include "PS-mcu client.h"
#include "clientInterface.h"
#include "clientData.h"

//===========================
// Global variables
//===========================

// Main menu block 
int mainMenuHandle;
int psMcuWindowHandles[PSMCU_MAX_NUM]; 

int SERVER_CONNECTION_INDICATOR;
int CANGW_CONNECTION_INDICATOR;

int PSMCU_WINDOW_BUTTONS[PSMCU_MAX_NUM];
int PSMCU_STATUS_INDICATOR[PSMCU_MAX_NUM];
int PSMCU_WINDOW_FIELDS_CAPTION;
int PSMCU_ORDER_LABELS[PSMCU_MAX_NUM];
int PSMCU_PIPELINE_STOP_BTN;

// Specific Interface elements for each power supply  
int PSMCU_ADC_GRAPH_BUTTONS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];

int PSMCU_BLOCK_ADC_FIELDS[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM]; 
int PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[PSMCU_MAX_NUM]; 
int PSMCU_BLOCK_DAC_FIELDS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM];
int PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[PSMCU_MAX_NUM]; 
int PSMCU_BLOCK_DAC_CONF_FIELDS[PSMCU_MAX_NUM][PSMCU_DAC_CHANNELS_NUM]; 
												
int PSMCU_BLOCK_INREG_INDICATORS[PSMCU_MAX_NUM][PSMCU_INREG_BITS_NUM]; 
int PSMCU_BLOCK_INREG_DUPLICATE_INDICATOR[PSMCU_MAX_NUM];
int PSMCU_BLOCK_OUTREG_INDICATORS[PSMCU_MAX_NUM][PSMCU_OUTREG_BITS_NUM];
int PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[PSMCU_MAX_NUM][2];
int PSMCU_BLOCK_INTERLOCK_RESET_BTNS[PSMCU_MAX_NUM]; 
int PSMCU_BLOCK_CURRENT_PERM_BTNS[PSMCU_MAX_NUM][2]; 
int PSMCU_BLOCK_FORCE_BTNS[PSMCU_MAX_NUM][2];
int PSMCU_BLOCK_ZERO_FAST_BTN[PSMCU_MAX_NUM];

int PSMCU_GRAPHS_WINDOW_HANDLES[PSMCU_MAX_NUM][PSMCU_ADC_CHANNELS_NUM];

//==============================================================================

void initValues(void) {
	int i,j;
	// ADCs 
	for (i=0; i < PSMCU_MAX_NUM; i++) {
		for (j=0;j < PSMCU_ADC_CHANNELS_NUM; j++) {
			ADC_STORED_VALS[i][j] = 0;			
		}
	}		
}

void setupSinglePsMcuGui(int psMcuIndex) {
	int i, j, k;
	int top, left;
	int maxHeight = 0;
	
	int w1 = 250, w2 = 250, w3 = 200;
	int output_indicators_order[PSMCU_OUTREG_BITS_NUM] = {0, 2, 1};
	
	// Prepare interface of a single PS-MCU window
		
	i = psMcuIndex;
	// The PS-MSU user interface has the following structure
	// Controls | ADC | Input Registries
	
	left = 5;
	top = 5;
	
	// =================
	// Control part
	// =================
	NewCtrl(psMcuWindowHandles[i], CTRL_TEXT_MSG, "Control (DAC):", top, left); 
	top += 20; 
	
	// DAC
	for (j=0; j < PSMCU_DAC_CHANNELS_NUM; j++) {
		// Labels
		PSMCU_BLOCK_DAC_FIELDS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_NUMERIC, PSMCU_ADC_LABELS_TEXT[i][j], top, left);
		PSMCU_BLOCK_DAC_CONF_FIELDS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_NUMERIC, "", top, left + ADC_ELEMENTS_STEP + DAC_FIELD_WIDTH);
		
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_WIDTH, DAC_FIELD_WIDTH);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_CTRL_MODE, VAL_HOT); 
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_PRECISION, 4);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_LABEL_TOP, top);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], ATTR_LABEL_LEFT, left + ADC_ELEMENTS_STEP + DAC_FIELD_WIDTH + ADC_ELEMENTS_STEP + DAC_CONF_FIELD_WIDTH); 
		
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_CONF_FIELDS[i][j], ATTR_WIDTH, DAC_CONF_FIELD_WIDTH);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_CONF_FIELDS[i][j], ATTR_CTRL_MODE, VAL_INDICATOR); 
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_CONF_FIELDS[i][j], ATTR_PRECISION, 4);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_CONF_FIELDS[i][j], ATTR_LABEL_TOP, top);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_CONF_FIELDS[i][j], ATTR_LABEL_LEFT, left + ADC_ELEMENTS_STEP + DAC_FIELD_WIDTH); 
		// Callback
		InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_DAC_FIELDS[i][j], dacFieldCallback, NULL);
		
		top += ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP;
	}
	
	// Output registries
	top += ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP;

	// Command buttons
	PSMCU_BLOCK_INTERLOCK_RESET_BTNS[i] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "Reset", top, left); 
	PSMCU_BLOCK_FORCE_BTNS[i][0] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "On", top + ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP, left);
	PSMCU_BLOCK_FORCE_BTNS[i][1] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "Off", top + ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP, left + CMD_BTN_WIDTH - (CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN)); 
	PSMCU_BLOCK_CURRENT_PERM_BTNS[i][0] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "On", top + 2 * (ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP), left);
	PSMCU_BLOCK_CURRENT_PERM_BTNS[i][1] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "Off", top + 2 * (ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP), left + CMD_BTN_WIDTH - (CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN));  
	
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[i], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[i], ATTR_WIDTH, CMD_BTN_WIDTH); 
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][0], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][0], ATTR_WIDTH, CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN); 
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][1], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][1], ATTR_WIDTH, CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN); 
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][0], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][0], ATTR_WIDTH , CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN); 
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][1], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
	SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][1], ATTR_WIDTH, CMD_BTN_WIDTH / 2 - CMD_BTN_MARGIN); 

	InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[i], interlockResetCmdBtnsCallback, NULL);
	InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][0], currentOnCmdBtnsCallback, NULL); 
	InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_CURRENT_PERM_BTNS[i][1], currentOffCmdBtnsCallback, NULL); 
	InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][0], forceOnCmdBtnsCallback, NULL); 
	InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_BLOCK_FORCE_BTNS[i][1], forceOffCmdBtnsCallback, NULL); 

	// Indicators
	for (k=0; k < PSMCU_OUTREG_BITS_NUM; k++) { 
		
		j = output_indicators_order[k];
		
		PSMCU_BLOCK_OUTREG_INDICATORS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_LED_LS, "", top, left + CMD_BTN_WIDTH + CMD_BTN_MARGIN);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_WIDTH, ADC_BUTTON_WIDTH); 
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_OFF_COLOR, VAL_GRAY);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_ON_COLOR, VAL_GREEN); 
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_LABEL_TOP, top + 3);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_OUTREG_INDICATORS[i][j], ATTR_LABEL_LEFT, left + 25 + CMD_BTN_WIDTH + CMD_BTN_MARGIN);
		top += ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP;
	}
	
	if (top + 5 > maxHeight) maxHeight = top + 5;
	
	// =================
	// ADC part
	// =================
	left = w1 + 5;
	top = 5;
	
	NewCtrl(psMcuWindowHandles[i], CTRL_TEXT_MSG, "ADC:", top, left); 
	top += 20; 
	
	for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
		PSMCU_GRAPHS_WINDOW_HANDLES[i][j] = -1;	
		PSMCU_ADC_GRAPH_BUTTONS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_SQUARE_COMMAND_BUTTON_LS, "Gr", top, left);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_ADC_GRAPH_BUTTONS[i][j], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_ADC_GRAPH_BUTTONS[i][j], ATTR_WIDTH, ADC_BUTTON_WIDTH); 
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_ADC_GRAPH_BUTTONS[i][j], ATTR_CMD_BUTTON_COLOR, MakeColor(239,235,250));

		// Labels
		PSMCU_BLOCK_ADC_FIELDS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_NUMERIC, PSMCU_ADC_LABELS_TEXT[i][j], top, left + ADC_BUTTON_WIDTH + ADC_ELEMENTS_STEP);
		
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_CTRL_MODE, VAL_INDICATOR);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_WIDTH, ADC_FIELD_WIDTH);
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_PRECISION, 4);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_LABEL_TOP, top);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_ADC_FIELDS[i][j], ATTR_LABEL_LEFT, left + ADC_BUTTON_WIDTH + ADC_FIELD_WIDTH + ADC_ELEMENTS_STEP * 2 + 5); 
		// Callback
		InstallCtrlCallback(psMcuWindowHandles[i], PSMCU_ADC_GRAPH_BUTTONS[i][j], adcButtonCallback, NULL);
		
		top += ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP;
	}
	
	if (top + 5 > maxHeight) maxHeight = top + 5;
	
	
	// =================
	// Input Registries part
	// =================
	left = w1 + w2 + 5;
	top = 5;
	
	NewCtrl(psMcuWindowHandles[i], CTRL_TEXT_MSG, "Input registries:", top, left); 
	top += 20; 
	
	for (j=0; j < PSMCU_INREG_BITS_NUM; j++) { 
		PSMCU_BLOCK_INREG_INDICATORS[i][j] = NewCtrl(psMcuWindowHandles[i], CTRL_ROUND_LED_LS, "", top, left);
		if (j <= 1) {
			SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_OFF_COLOR, VAL_RED);
			SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_ON_COLOR, VAL_GREEN); 	
		} else {
			SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_OFF_COLOR, VAL_GRAY);
			SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_ON_COLOR, VAL_RED); 	
		}
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_LABEL_TOP, top + 3);  
		SetCtrlAttribute(psMcuWindowHandles[i], PSMCU_BLOCK_INREG_INDICATORS[i][j], ATTR_LABEL_LEFT, left + 25);
		top += ADC_BUTTON_HEIGHT + ADC_ELEMENTS_STEP;
	}

	// Set the final window size
	SetPanelAttribute(psMcuWindowHandles[i], ATTR_HEIGHT, maxHeight);
	SetPanelAttribute(psMcuWindowHandles[i], ATTR_WIDTH, w1 + w2 + w3);	
}


void initGui(void) {
	int i;
	int top, left, width;
	int elementHandle;
	char string[256];
	
	//////////////////////////////////
	// Main menu timer
	//////////////////////////////////
	SetCtrlAttribute(mainMenuHandle, BlockMenu_TIMER, ATTR_INTERVAL,TIMER_TICK_TIME);
	//////////////////////////////////
	// Main menu buttons
	//////////////////////////////////

	top = 5;
	left = 5;

	SERVER_CONNECTION_INDICATOR = NewCtrl(mainMenuHandle, CTRL_SQUARE_LED_LS, SERVER_INDICATOR_OFFLINE_LABEL, top, left);
	SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_OFF_COLOR, VAL_RED);
	SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_ON_COLOR, VAL_GREEN); 
	SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_LABEL_TOP, top + 3);  
	SetCtrlAttribute(mainMenuHandle, SERVER_CONNECTION_INDICATOR, ATTR_LABEL_LEFT, left + 40); 
	top += 24;
	
	CANGW_CONNECTION_INDICATOR = NewCtrl(mainMenuHandle, CTRL_SQUARE_LED_LS, CANGW_INDICATOR_OFFLINE_LABEL, top, left);
	SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_OFF_COLOR, VAL_RED);
	SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_ON_COLOR, VAL_GREEN); 
	SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_TOP, top + 3);  
	SetCtrlAttribute(mainMenuHandle, CANGW_CONNECTION_INDICATOR, ATTR_LABEL_LEFT, left + 40);
	top += 32;  
	
	// Captions
	left = 20;
	NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "Online", top, left); left += 40 + BLOCK_BUTTON_WIDTH;
	NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "Cont.", top, left - 8); left += 25; 
	NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "F", top, left); left += 25; 
	NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "C", top, left); left += 25; 
	PSMCU_WINDOW_FIELDS_CAPTION = NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "Undefined name", top, left);  left += 144;  
	NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, "(use double click)", top, left); left += 25;
	
	top += 20;
	
	// PSMCUs
	for (i=0; i < PSMCU_NUM; i++) {
		left = 5;
		
		// Order hint
		sprintf(string, "%d", PSMCU_DEVICES_PRIORITY[i] + 1);
		PSMCU_ORDER_LABELS[i] = NewCtrl(mainMenuHandle, CTRL_TEXT_MSG, string, top, left);
		left += 20;
		
		// STATUS Indicator
		PSMCU_STATUS_INDICATOR[i] = NewCtrl(mainMenuHandle, CTRL_SQUARE_LED_LS, "", top, left);
		SetCtrlAttribute(mainMenuHandle, PSMCU_STATUS_INDICATOR[i], ATTR_CTRL_MODE, VAL_INDICATOR);
		SetCtrlAttribute(mainMenuHandle, PSMCU_STATUS_INDICATOR[i], ATTR_OFF_COLOR, VAL_RED);
		SetCtrlAttribute(mainMenuHandle, PSMCU_STATUS_INDICATOR[i], ATTR_ON_COLOR, VAL_GREEN); 
		SetCtrlAttribute(mainMenuHandle, PSMCU_STATUS_INDICATOR[i], ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, PSMCU_STATUS_INDICATOR[i], ATTR_WIDTH, 20);
		
		left += 25;
		
		// buttons style
		PSMCU_WINDOW_BUTTONS[i] = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, PSMCU_DEV_TITLE[i], top, left);
		SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[i], ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[i], ATTR_WIDTH, BLOCK_BUTTON_WIDTH); 
		SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[i], ATTR_CMD_BUTTON_COLOR, MakeColor(249,136,136));
		SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[i], ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE);  
		
		// installing block button callback
		InstallCtrlCallback(mainMenuHandle, PSMCU_WINDOW_BUTTONS[i],mainMenuButtonCallback,NULL);
		
		left += BLOCK_BUTTON_WIDTH + 5;
		
		// Contactor

		elementHandle = NewCtrl(mainMenuHandle, CTRL_ROUND_LED_LS, "", top, left); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_INDICATOR);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_OFF_COLOR, VAL_RED);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_ON_COLOR, VAL_GREEN); 
		left += 25; 
		PSMCU_BLOCK_INREG_DUPLICATE_INDICATOR[i] = elementHandle;
		
		// Permissions indicators 
	
		elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_LED_LS, "", top, left);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_INDICATOR);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_OFF_COLOR, VAL_GRAY);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_ON_COLOR, VAL_GREEN); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, 20);
		left += 25;
		
		PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[i][0] = elementHandle;
		
		elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_LED_LS, "", top, left);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_INDICATOR);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_OFF_COLOR, VAL_GRAY);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_ON_COLOR, VAL_GREEN); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, 20);
		left += 25;
		
		PSMCU_BLOCK_OUTREG_DUPLICATE_INDICATORS[i][1] = elementHandle;
		
		// DAC duplicate field
		elementHandle = NewCtrl(mainMenuHandle, CTRL_NUMERIC_LS, "", top, left);
		
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, DAC_FIELD_WIDTH);  
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_HOT); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_PRECISION, 4);  
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_FRAME_COLOR, VAL_WHITE);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_TEXT_BGCOLOR, VAL_WHITE);
		// Callback
		InstallCtrlCallback(mainMenuHandle, elementHandle, dacFieldCallback, NULL);
		
		PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[i] = elementHandle;
		
		left += DAC_FIELD_WIDTH + 5;
		
		// ADC duplicate field
		elementHandle = NewCtrl(mainMenuHandle, CTRL_NUMERIC_LS, "", top, left);
		
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, ADC_FIELD_WIDTH);  
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_INDICATOR); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_PRECISION, 4);  
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_FRAME_COLOR, VAL_WHITE);
		
		PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[i] = elementHandle;

		left += ADC_FIELD_WIDTH + 5;
		
		// Fast zero button
		PSMCU_BLOCK_ZERO_FAST_BTN[i] = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, "Zero DAC fast", top, left); 
		
		SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ZERO_FAST_BTN[i], ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE);
		SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ZERO_FAST_BTN[i], ATTR_HEIGHT, ADC_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ZERO_FAST_BTN[i], ATTR_WIDTH, 85); 
		
		InstallCtrlCallback(mainMenuHandle, PSMCU_BLOCK_ZERO_FAST_BTN[i], zeroSingleFastBtnCallback, NULL); 
		
		
		left += 85 + 5; 
		
		// End of the main controls of a singel block (in the Main window)
		
		top += BLOCK_BUTTON_HEIGHT + 2;
	}
	
	width = left;
	
	// Broadcast command buttons
	top += 5;
	elementHandle = NewCtrl(mainMenuHandle, CTRL_HORIZONTAL_SPLITTER, "", top, 0);
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, width);  
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_FRAME_COLOR, MakeColor(200, 200, 200));
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CTRL_MODE, VAL_INDICATOR);   
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_OPERABLE_AS_INDICATOR, 0);  
	
	top += 14; 
	
	// Setup DACs
	for (i=0; i < 3; i++) {
		switch(i) {
			case 0:
				elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, "Save", top, 5 + i * (CMD_BTN_WIDTH + 2));
				InstallCtrlCallback(mainMenuHandle, elementHandle, saveDacBtnCallback, NULL); 
				break;
			case 1:
				elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, "Load", top, 5 + i * (CMD_BTN_WIDTH + 2));
				InstallCtrlCallback(mainMenuHandle, elementHandle, loadDacBtnCallback, NULL); 
				break;
			case 2:
				elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, "Stop", top, 5 + i * (CMD_BTN_WIDTH + 2));
				InstallCtrlCallback(mainMenuHandle, elementHandle, stopPipelineCallback, NULL); 
				SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_DIMMED, 1);
				PSMCU_PIPELINE_STOP_BTN = elementHandle; 
				break;
		}
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, CMD_BTN_WIDTH); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CMD_BUTTON_COLOR, MakeColor(200,200,200));
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE); 
	}
	
	for (i=0; i < 2; i++) {
		elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, (i == 0) ? "Smart All Zero DAC (dbl click)" : "Fast All Zero DAC (dbl click)", top + (i + 1) * (BLOCK_BUTTON_HEIGHT + 2), 5); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, (CMD_BTN_WIDTH + 2) * 3 - 2); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CMD_BUTTON_COLOR, (i == 0) ? MakeColor(200,200,200) : MakeColor(206,157,230));
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE); 	
		
		InstallCtrlCallback(mainMenuHandle, elementHandle, (i == 0) ? zeroAllSmartBtnCallback : zeroAllFastBtnCallback, NULL); 
	}
	
	// Control registers
	elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, "Reset All", top, width - 5 - (CMD_BTN_WIDTH_2 * 2 + 2)); 
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, CMD_BTN_WIDTH_2 * 2 + 2); 
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CMD_BUTTON_COLOR, MakeColor(200,200,200));
	SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE); 
	InstallCtrlCallback(mainMenuHandle, elementHandle, allResetBtnCallback, NULL);
	
	for (i=0; i < 2; i++) {
		elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, (i == 0) ? "Force On" : "Force Off", top + (BLOCK_BUTTON_HEIGHT + 2), width - 5 - (CMD_BTN_WIDTH_2 * 2 + 2) + i * (CMD_BTN_WIDTH_2 + 2)); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, CMD_BTN_WIDTH_2); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CMD_BUTTON_COLOR, MakeColor(200,200,200));
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE); 	
		InstallCtrlCallback(mainMenuHandle, elementHandle, (i == 0) ? allForceOnBtnCallback : allForceOffBtnCallback, NULL); 
	}
	
	for (i=0; i < 2; i++) {
		elementHandle = NewCtrl(mainMenuHandle, CTRL_SQUARE_COMMAND_BUTTON_LS, (i == 0) ? "Current On" : "Current Off", top + 2 * (BLOCK_BUTTON_HEIGHT + 2), width - 5 - (CMD_BTN_WIDTH_2 * 2 + 2) + i * (CMD_BTN_WIDTH_2 + 2)); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_HEIGHT, BLOCK_BUTTON_HEIGHT);
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_WIDTH, CMD_BTN_WIDTH_2); 
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_CMD_BUTTON_COLOR, MakeColor(200,200,200));
		SetCtrlAttribute(mainMenuHandle, elementHandle, ATTR_AUTO_SIZING, VAL_NEVER_AUTO_SIZE);
		InstallCtrlCallback(mainMenuHandle, elementHandle, (i == 0) ? allCurrentOnBtnCallback : allCurrentOffBtnCallback, NULL);
	}
	
	top += 3 * (BLOCK_BUTTON_HEIGHT + 2);
	
	// Main menu style
	SetPanelAttribute(mainMenuHandle, ATTR_HEIGHT, top + 3);
	SetPanelAttribute(mainMenuHandle, ATTR_WIDTH, width);
	
	
	//////////////////////////////////
	// PS-MCU individual windows
	//////////////////////////////////
	for (i=0; i < PSMCU_NUM; i++) {
		setupSinglePsMcuGui(i);	 
	}
}


void DiscardGuiResources(void) {
	int i,j; 
	
	if (mainMenuHandle >= 0) DiscardPanel (mainMenuHandle);
	for (i=0; i < PSMCU_NUM; i++) {
		if (psMcuWindowHandles[i] >= 0) DiscardPanel (psMcuWindowHandles[i]); 
		for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
			if (PSMCU_GRAPHS_WINDOW_HANDLES[i][j] >= 0)
				DiscardPanel (PSMCU_GRAPHS_WINDOW_HANDLES[i][j]); 	
		}
	}
}


void UpdateGraphs(void) {
	int i,j;
	for (i=0; i < PSMCU_NUM; i++) {
		for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
			if (PSMCU_GRAPHS_WINDOW_HANDLES[i][j] < 0) continue;
			
			PlotStripChartPoint(PSMCU_GRAPHS_WINDOW_HANDLES[i][j], Graph_GRAPH, ADC_STORED_VALS[i][j]);
			SetCtrlVal(PSMCU_GRAPHS_WINDOW_HANDLES[i][j], Graph_currentValue, ADC_STORED_VALS[i][j]);
		}
	}
}


void UpdateDeviceName(int deviceIndex, char *name) {
	int channelIndex;
	strcpy(PSMCU_DEV_NAME[deviceIndex], name);
	sprintf(PSMCU_DEV_TITLE[deviceIndex], "%d: %s", deviceIndex + 1, PSMCU_DEV_NAME[deviceIndex]);
	SetPanelAttribute(psMcuWindowHandles[deviceIndex], ATTR_TITLE, PSMCU_DEV_TITLE[deviceIndex]);
	SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[deviceIndex], ATTR_LABEL_TEXT, PSMCU_DEV_TITLE[deviceIndex]); 
	for (channelIndex=0; channelIndex < PSMCU_ADC_CHANNELS_NUM; channelIndex++) {
		UpdateGraphTitle(deviceIndex, channelIndex);	
	}
}


void UpdateGraphTitle(int deviceIndex, int channelIndex) {
	char graphTitle[256];
	if (PSMCU_GRAPHS_WINDOW_HANDLES[deviceIndex][channelIndex] < 0) return;
	sprintf(graphTitle, "%s - ADC channel %d: %s", PSMCU_DEV_TITLE[deviceIndex], channelIndex, PSMCU_ADC_LABELS_TEXT[deviceIndex][channelIndex]);
	SetPanelAttribute(PSMCU_GRAPHS_WINDOW_HANDLES[deviceIndex][channelIndex], ATTR_TITLE, graphTitle);
}


int CVICALLBACK adcPanelCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_CLOSE:
			HidePanel(panel);
			break;
	}
	return 0;
}


int CVICALLBACK mainMenuButtonCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	
	int i;

	switch (event)
	{
		case EVENT_COMMIT:
			for (i=0; i < PSMCU_NUM; i++) {
				if (PSMCU_WINDOW_BUTTONS[i] == control) {
					DisplayPanel(psMcuWindowHandles[i]);
					break;
				}
			}
			break;
	}
	return 0;
}


int CVICALLBACK graphCallback (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int i, j;
	int h, w;
	switch (event) {
		case EVENT_PANEL_SIZE:  
			GetPanelAttribute(panel, ATTR_HEIGHT, &h);
			if (h<150) SetPanelAttribute(panel, ATTR_HEIGHT, 150);
			GetPanelAttribute(panel, ATTR_WIDTH, &w);
			if (w<300) SetPanelAttribute(panel, ATTR_WIDTH, 300);
		case EVENT_PANEL_SIZING:
			
			GetPanelAttribute(panel, ATTR_WIDTH, &w);
			GetPanelAttribute(panel, ATTR_HEIGHT, &h);
			SetCtrlAttribute(panel, Graph_GRAPH, ATTR_WIDTH, w-113);
			SetCtrlAttribute(panel, Graph_GRAPH, ATTR_HEIGHT, h); 
			SetCtrlAttribute(panel, Graph_currentValue, ATTR_TOP, h/2); 
			SetCtrlAttribute(panel, Graph_minValue, ATTR_TOP, h-30);
			break;
		case EVENT_CLOSE:
			DiscardPanel(panel);
			for (i=0; i < PSMCU_NUM; i++) {
				for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
					if (PSMCU_GRAPHS_WINDOW_HANDLES[i][j] == panel) {
						PSMCU_GRAPHS_WINDOW_HANDLES[i][j] = -1;
						break;
					}
				}
				if (j < PSMCU_ADC_CHANNELS_NUM) break;
			}
			break;
	}  
	return 0;
}


int CVICALLBACK adcButtonCallback (int panel, int control, int event, void *callbackData,
		int eventData1, int eventData2) {
	
	int i,j;
	
	switch (event) {
		case EVENT_COMMIT:
			for (i=0; i < PSMCU_NUM; i++) {
				if (psMcuWindowHandles[i] == panel) break;	
			}
			if (i >= PSMCU_NUM) break;
			
			for (j=0; j < PSMCU_ADC_CHANNELS_NUM; j++) {
				if (PSMCU_ADC_GRAPH_BUTTONS[i][j] == control) {
					if (PSMCU_GRAPHS_WINDOW_HANDLES[i][j] < 0) {
						PSMCU_GRAPHS_WINDOW_HANDLES[i][j] = LoadPanel(0,"PS-mcu client.uir",Graph);
						if (PSMCU_GRAPHS_WINDOW_HANDLES[i][j] < 0) break;
						UpdateGraphTitle(i, j);
						DisplayPanel(PSMCU_GRAPHS_WINDOW_HANDLES[i][j]);
					}
					DisplayPanel(PSMCU_GRAPHS_WINDOW_HANDLES[i][j]);
					break;
				}
			}
			break;
	}
	return 0;
}


int CVICALLBACK graphVerticalRange (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double ymin, ymax;
	double buf;
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel,Graph_maxValue,&ymax);
			GetCtrlVal(panel,Graph_minValue,&ymin);
			if (ymax < ymin)  {
				buf = ymax;
				ymax = ymin;
				ymin = buf;
				SetCtrlVal(panel,Graph_maxValue,ymax);
				SetCtrlVal(panel,Graph_minValue,ymin);
			}
			
			if (ymax == ymin) {
				ymax += 1;
				SetCtrlVal(panel,Graph_maxValue,ymax);
			}
			
			SetAxisScalingMode(panel, Graph_GRAPH, VAL_LEFT_YAXIS, VAL_MANUAL, ymin, ymax);
			break;
	}	 
	return 0;
}


int CVICALLBACK panelCB (int panel, int event, void *callbackData,
        int eventData1, int eventData2)
{
    if (event == EVENT_CLOSE) {
        QuitUserInterface (0);
	}
    return 0;
}


void resolveCommandButtonStatus(int deviceIndex, unsigned int inputRegisters, unsigned int outputRegisters, double dacVoltage) {
	int interlockReset, currentPermission, forcePermission, contactor;
	
	int forceOnBtnDimmed, currentOnBtnDimmed, resetBtnDimmed;
	
	forceOnBtnDimmed = 0;
	currentOnBtnDimmed = 0;
	resetBtnDimmed = 0;
	
	interlockReset = (outputRegisters >> 0) & 1;
	currentPermission = (outputRegisters >> 1) & 1;
	forcePermission = (outputRegisters >> 2) & 1;
	contactor = (inputRegisters >> 1) & 1;
	
	if (currentPermission || forcePermission) resetBtnDimmed = 1;
	if (interlockReset) {
		forceOnBtnDimmed = 1;
		currentOnBtnDimmed = 1;
	}
	
	if (currentPermission) {
		forceOnBtnDimmed = 1;		
	}
	
	if ( !contactor || !forcePermission ) currentOnBtnDimmed = 1;
	
	if (dacVoltage != 0) currentOnBtnDimmed = 1; 
	
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_INTERLOCK_RESET_BTNS[deviceIndex], ATTR_DIMMED, resetBtnDimmed);
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_CURRENT_PERM_BTNS[deviceIndex][0], ATTR_DIMMED, currentOnBtnDimmed);
	SetCtrlAttribute(psMcuWindowHandles[deviceIndex], PSMCU_BLOCK_FORCE_BTNS[deviceIndex][0], ATTR_DIMMED, forceOnBtnDimmed);
}


void resolvePsmcuColorIndicators(
	int deviceIndex,
	unsigned int inputRegisters,
	double dacSentVoltage,
	double dacReadVoltage,
	double adcVoltage,
	double dac_dac_max_diff,
	double dac_adc_max_diff
) {
	static int previousFaultIndication[PSMCU_MAX_NUM];
	static int previousDacIndication[PSMCU_MAX_NUM];  
	static int previousAdcIndication[PSMCU_MAX_NUM];
	static int initialized = 0;
	int tmpDevIndex;
	
	// 1 - apply special indication
	// 0 - no special indication requierd
	int faultIndication, dacIndication, adcIndication;
	double dac_dac_diff, dac_adc_diff;
	
	// Set initial state
	if (!initialized) {
		initialized = 1;
		for (tmpDevIndex=0; tmpDevIndex < PSMCU_MAX_NUM; tmpDevIndex++) {
			previousFaultIndication[tmpDevIndex] = 1;
			previousDacIndication[tmpDevIndex] = 0;
			previousAdcIndication[tmpDevIndex] = 0;
		}
	}
	
	// determine current states
	if ((inputRegisters & 0xd) != 0x1) {
		faultIndication = 1;	
	} else {
		faultIndication = 0;
	}
	
	dac_dac_diff = dacSentVoltage - dacReadVoltage;
	dac_adc_diff = dacReadVoltage - adcVoltage;
	
	if (dac_dac_diff < 0) dac_dac_diff = -dac_dac_diff;
	if (dac_adc_diff < 0) dac_adc_diff = -dac_adc_diff;
	
	if (dac_dac_diff > dac_dac_max_diff) {
		dacIndication = 1;	
	} else {
		dacIndication = 0;	
	}
	
	if (dac_adc_diff > dac_adc_max_diff) {
		adcIndication = 1;	
	} else {
		adcIndication = 0;	
	}
	
	// Update interface if necessary
	if (previousFaultIndication[deviceIndex] != faultIndication) {
		if (faultIndication)
			SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[deviceIndex], ATTR_CMD_BUTTON_COLOR, MakeColor(249,136,136));   
		else
			SetCtrlAttribute(mainMenuHandle, PSMCU_WINDOW_BUTTONS[deviceIndex], ATTR_CMD_BUTTON_COLOR, MakeColor(190,247,187));  
	}
	
	if (previousDacIndication[deviceIndex] != dacIndication) {
		if (dacIndication) {
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], ATTR_FRAME_COLOR, MakeColor(228,228,20)); 
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], ATTR_TEXT_BGCOLOR, MakeColor(255,255,145)); 
		} else {
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], ATTR_FRAME_COLOR, VAL_WHITE);
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_DAC_DUPLICATE_FIELDS[deviceIndex], ATTR_TEXT_BGCOLOR, VAL_WHITE);	
		}
	}
	
	if (previousAdcIndication[deviceIndex] != adcIndication) {
		if (adcIndication) {
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ATTR_FRAME_COLOR, MakeColor(249,136,136));
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ATTR_TEXT_BGCOLOR, MakeColor(249,136,136));
		} else {
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ATTR_FRAME_COLOR, VAL_WHITE);
			SetCtrlAttribute(mainMenuHandle, PSMCU_BLOCK_ADC_CURRENT_DUPLICATE_FIELDS[deviceIndex], ATTR_TEXT_BGCOLOR, VAL_WHITE);	
		}
	}
	
	// Update the stored states
	previousFaultIndication[deviceIndex] = faultIndication;
	previousDacIndication[deviceIndex] = dacIndication;
	previousAdcIndication[deviceIndex] = adcIndication;
}
