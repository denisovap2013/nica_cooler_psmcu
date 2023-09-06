//==============================================================================
//
// Title:       LoadSave.c
// Purpose:     A short description of the implementation.
//
// Created on:  08.02.2022 at 10:36:31 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "inifile.h"
#include <ansi_c.h>

#include "LoadSave.h"
#include "clientData.h"

//==============================================================================
// Constants
#define VALUES_SECTION "VALUES"
#define COMMENTS_SECTION "COMMENTS"
//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions

//==============================================================================
// Global variables

//==============================================================================
// Global functions

void SaveCurrentState(void) {
	char file_path[1024];
	IniText iniText; 
	int deviceIndex; 
	char key[256]; 
	
	if (FileSelectPopup("", "*.ini", "*.ini", "Select the configuration file", VAL_SAVE_BUTTON, 0, 1, 1, 1, file_path) == 0) return;
	
	iniText = Ini_New(0);
	
	// Save PSMCU_NUM values
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
		sprintf(key, "device_%d", deviceIndex);
		Ini_PutDouble(iniText, VALUES_SECTION, key, DAC_CLIENT_SENT_VALUES[deviceIndex][0]); 
		Ini_PutString(iniText, COMMENTS_SECTION, key, PSMCU_DEV_NAME[deviceIndex]); 
	}
	
	// Trying to open the file for writing
	if( Ini_WriteToFile(iniText, file_path) < 0 ) {
		MessagePopup("Unable to save the currents", "Unable to open the specified file for writing.");
		Ini_Dispose(iniText);
		return;
	}
	
	// Free the resources
	Ini_Dispose(iniText);
}


void LoadCurrentState(void (*dacSetupFunction)(double dacStates[PSMCU_MAX_NUM])) {
	char file_path[1024];
	IniText iniText; 
	int deviceIndex; 
	char key[256], msg[256];
	double currents[PSMCU_MAX_NUM];
	
	if (FileSelectPopup("", "*.ini", "*.ini", "Select the configuration file", VAL_LOAD_BUTTON, 0, 1, 1, 1, file_path) == 0) return;
	
	iniText = Ini_New(0);
	
	// Trying to open the file for reading
	if( Ini_ReadFromFile(iniText, file_path) != 0 ) {
		MessagePopup("Unable to load the currents", "Unable to open the specified file for reading.");
		Ini_Dispose(iniText);
		return;
	}
	
	// Load the currents from file
	for (deviceIndex=0; deviceIndex < PSMCU_NUM; deviceIndex++) {
		sprintf(key, "device_%d", deviceIndex);
		if (Ini_GetDouble(iniText, VALUES_SECTION, key, &currents[deviceIndex]) <= 0) {
			sprintf(msg, "Unable to read a double value from the field \"%s\" from the configuration file.", key);
			MessagePopup("Unable to load the currents", msg);
			Ini_Dispose(iniText);	
			return;
		}
	}
	
	// Call the user specified function
	dacSetupFunction(currents);
	
	// Free the resources
	Ini_Dispose(iniText);
}
