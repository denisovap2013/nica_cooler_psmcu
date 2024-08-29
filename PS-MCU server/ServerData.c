//==============================================================================
//
// Title:       ServerData.c
// Purpose:     A short description of the implementation.
//
// Created on:  15.12.2021 at 13:22:13 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include <ansi_c.h>
#include "cangw.h"

#include "TCP_Connection.h"
#include <ansi_c.h>
#include "CGW_Devices.h"
#include "CGW_Connection.h"
#include "CGW_PSMCU_Interface.h" 

#include "ServerData.h"

//==============================================================================
// Constants
cgw_devices_t deviceKit[CFG_CGW_MAX_NUM];
tcpConnection_ServerInterface_t tcpSI = {0};

#define PERMISSION_MASK 0x06
#define INTERLOCK_MASK 0x01
#define NO_REQUIREMENTS_MASK 0x00
#define NO_REQUIREMENTS_STATE 0x00
#define TURNED_OFF_STATE 0x00    

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
#define CHECK_DEVICE_ID(devId, devKit) if ((devId) <= 0 || (devId) > 0xFF) return -1; else {if(!(devKit).registered[(devId)]) return -1;}
#define CHECK_ADC_CHANNEL(ch) if ((ch) < 0 || (ch) >= PSMCU_ADC_CHANNELS_NUM) {return -1;}
#define CHECK_DAC_CHANNEL(ch) if ((ch) < 0 || (ch) >= PSMCU_DAC_CHANNELS_NUM) {return -1;} 
#define CHECK_INREG_INDEX(index) if ((index) < 0 || (index) >= PSMCU_INPUT_REGISTERS_NUM) {return -1;}
#define CHECK_OUTREG_INDEX(index) if ((index) < 0 || (index) >= PSMCU_OUTPUT_REGISTERS_NUM) {return -1;}  

int controlSingleSetSingleOutputBit(int cgwIndex, int deviceId, int bit, unsigned char value);
int controlAllSetSingleOutputBit(int cgwIndex, int bit, unsigned char value);  
//==============================================================================
// Global variables

//==============================================================================
// Global functions

int deviceIndexToDeviceId(int globalDeviceIndex, int *cgwIndex, int *deviceId) {
	if (globalDeviceIndex < 0 || globalDeviceIndex >= CFG_PSMCU_DEVICES_NUM) return -1;
	*cgwIndex = CFG_PSMCU_BLOCKS_IDS[globalDeviceIndex];
	*deviceId = CFG_PSMCU_DEVICES_IDS[globalDeviceIndex];
	return 0;
}


int getDeviceFullInfo(int cgwIndex, int deviceId, char *formattedInfo) {
	int chIndex, deviceIndex;
	char buffer[256];
	double doubleBuffer;
	unsigned int status;

	cgwPsMcu_Information_t *deviceParameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	
	formattedInfo[0] = 0;  // empty the string
	deviceParameters = (cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId];
	
	deviceIndex = deviceKit[cgwIndex].idsToIndicesMap[deviceId];  // TODO: fix index search
	
	// ADC info
	for (chIndex = 0; chIndex < PSMCU_ADC_CHANNELS_NUM; chIndex++) {
		sprintf(buffer, "%.8lf ", deviceParameters->ChannelVoltage[chIndex] * deviceParameters->AdcCoefficients[chIndex][0] + deviceParameters->AdcCoefficients[chIndex][1]);
		strcat(formattedInfo, buffer);  // Append the string to the formatted info
	}
	
	// DAC info
	for (chIndex = 0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) {  
		doubleBuffer = deviceParameters->DACvoltage[0];
		if (deviceParameters->DacCoefficients[chIndex][0] != 0) {
			doubleBuffer = (doubleBuffer - deviceParameters->DacCoefficients[chIndex][1]) / deviceParameters->DacCoefficients[chIndex][0];	
		}
	
		sprintf(buffer, "%.8lf ", doubleBuffer);
		strcat(formattedInfo, buffer);  // Append the string to the formatted info
	}
	
	// Input and output registers
	sprintf(buffer, "%X %X ", deviceParameters->InputRegisterData, deviceParameters->OutputRegisterData);
	strcat(formattedInfo, buffer);
	
	// 1st bit - alive status, 2nd bit - error status
	getDeviceStatus(cgwIndex, deviceId, &status);
	sprintf(buffer, "%X", status);
	strcat(formattedInfo, buffer);

	return 0;
}


int getDeviceStatus(int cgwIndex, int deviceId, unsigned int *status) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);

	*status = deviceKit[cgwIndex].active[deviceId] & (deviceKit[cgwIndex].error_state[deviceId] << 1);

	return 0;
}


int setDacVoltage(int cgwIndex, int deviceId, int channel, double voltage, int raw, int slow) {
    #define ALLOWED_OVERFLOW 1.1
	cgwPsMcu_Information_t *deviceParameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_DAC_CHANNEL(channel); 
	
	deviceParameters = (cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId]; 
	
	if (raw <= 0)voltage = voltage * deviceParameters->DacCoefficients[channel][0] + deviceParameters->DacCoefficients[channel][1]; 

	if (voltage < -10.) {
		if (voltage < -10.0 * ALLOWED_OVERFLOW) return -1;
		voltage = -10.;
	}
	if (voltage > 9.9997) {
		if (voltage > 9.9997 * ALLOWED_OVERFLOW) return -1;
		voltage = 9.9997;
	}
	
	if (slow) {
		deviceParameters->usingDACslowMode[channel] = 1;
		cgwPsMcu_DACVoltageToCode(voltage, &deviceParameters->targetDACcode[channel]);
		deviceParameters->DAClastTimeWrite[channel] = clock() * 1000 / CLOCKS_PER_SEC;
	} else {
		deviceParameters->usingDACslowMode[channel] = 0;
		deviceParameters->DACcodeConfirmed[channel] = 0;
		cgwPsMcu_DACWriteVoltage(cgwIndex, deviceId, channel, voltage);	
	}
	
	return 0;
}


int getAdcVoltage(int cgwIndex, int deviceId, int channel, double *result, int raw) {
	double voltage;
	cgwPsMcu_Information_t *deviceParameters;

	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_ADC_CHANNEL(channel); 
	
	deviceParameters = (cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId]; 
	
	voltage = deviceParameters->ChannelVoltage[channel]; 
	
	if (raw <= 0) voltage = voltage * deviceParameters->AdcCoefficients[channel][0] + deviceParameters->AdcCoefficients[channel][1];
	
	(*result) = voltage;
	
	return 0;
}


int getAllRegisters(int cgwIndex, int deviceId, unsigned char *inputRegisters, unsigned char *outputRegisters) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);

	(*inputRegisters) = ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->InputRegisterData;
	(*outputRegisters) = ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->OutputRegisterData;
	
	return 0;
}


int setOutputRegisters(int cgwIndex, int deviceId, unsigned char registers, unsigned char mask) {
	unsigned char prevReg, newReg;
	cgwPsMcu_Information_t *parameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	
	parameters = deviceKit[cgwIndex].parameters[deviceId];
	
	if (!parameters->OutputRegistersConfirmed) return 0;

	parameters->OutputRegistersConfirmed = 0;
	prevReg = parameters->OutputRegisterData;
	newReg = prevReg & (0xFF - mask) | registers & mask;
	
	cgwPsMcu_WriteToOutputRegister(cgwIndex, deviceId, newReg);
	cgwPsMcu_RegistersRequest(cgwIndex, deviceId);

	return 0;
}


int resetSingleAdcMeasurements(int cgwIndex, int deviceId) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	cgwPsMcu_MultiChannelConfig(cgwIndex, deviceId, 0, PSMCU_ADC_CHANNELS_NUM - 1, PSMCU_TIME_20MS, PSMCU_MODE_CONTINUOUS | PSMCU_MODE_TAKEOUT, 0);  
	return 0;
}


int resetAllAdcMeasurements(int cgwIndex) {
	int deviceIndex, deviceId;
	
	for (deviceIndex=0; deviceIndex < deviceKit[cgwIndex].registeredNum; deviceIndex ++) {
		deviceId = deviceKit[cgwIndex].registeredIDs[deviceIndex];
		cgwPsMcu_MultiChannelConfig(cgwIndex, deviceId, 0, PSMCU_ADC_CHANNELS_NUM - 1, PSMCU_TIME_20MS, PSMCU_MODE_CONTINUOUS | PSMCU_MODE_TAKEOUT, 0); 
	}
	
	return 0;
}


int stopSingleAdcMeasurements(int cgwIndex, int deviceId) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	cgwPsMcu_Stop(cgwIndex, deviceId, 0);
	return 0;
}


int stopAllAdcMeasurements(int cgwIndex) {
	cgwPsMcu_Stop(cgwIndex, deviceKit[cgwIndex].registeredIDs[0], 1);
	return 0;	
}


int getAdcChannelName(int cgwIndex, int deviceId, int channel, char *buffer) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_ADC_CHANNEL(channel);
	strcpy(buffer, ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->AdcChannelsNames[channel]);  
	return 0;
}


int getDacChannelName(int cgwIndex, int deviceId, int channel, char *buffer) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_DAC_CHANNEL(channel);
	strcpy(buffer, ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->DacChannelsNames[channel]);
	return 0;
}


int getInputRegisterName(int cgwIndex, int deviceId, int registerIndex, char *buffer) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_INREG_INDEX(registerIndex);
	strcpy(buffer, ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->InputRegistersNames[registerIndex]);
	return 0;
}


int getOutputRegisterName(int cgwIndex, int deviceId, int registerIndex, char *buffer) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	CHECK_OUTREG_INDEX(registerIndex);
	strcpy(buffer, ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->OutputRegistersNames[registerIndex]);
	return 0;
}


int getDeviceName(int cgwIndex, int deviceId, char *buffer) {
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	strcpy(buffer, ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->deviceName);
	return 0;	
}


int setAllDacToZero(int cgwIndex) {
	int deviceIndex, deviceId;
	int chIndex;

	for (deviceIndex=0; deviceIndex < deviceKit[cgwIndex].registeredNum; deviceIndex++) {
		deviceId = deviceKit[cgwIndex].registeredIDs[deviceIndex];
		for (chIndex=0; chIndex < PSMCU_DAC_CHANNELS_NUM; chIndex++) {
			setDacVoltage(cgwIndex, deviceId, chIndex, 0, 1, 1); 
		}
	}
	return 0;		
}


int controlSingleForceOn(int cgwIndex, int deviceId) {
	// Check the Current Permission is set to 0
	cgwPsMcu_Information_t *parameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);

	parameters = deviceKit[cgwIndex].parameters[deviceId];
	
	// If the Current permission is ON, unable to set the Force permission to 1 (incorrect order) 
	if ((parameters->OutputRegisterData >> 1) & 1) return 0;

	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 2, 1);	
}


int controlSingleForceOff(int cgwIndex, int deviceId) {
	// No bits check is necessary  
	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 2, 0);	
}


int controlSinglePermissionOn(int cgwIndex, int deviceId) {
	// Check the Contactor input bit is set to 1 and the DAC voltage is set to 0 (0x8000).
	cgwPsMcu_Information_t *parameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);

	parameters = deviceKit[cgwIndex].parameters[deviceId];
	
	// If Force permission bit is not set to 1, unable to set the Current permission to 1
	if (((parameters->OutputRegisterData >> 2) & 1) != 1) return 0;
	
	// If "Contactor" bit is not set to 1, unable to set the Current permission to 1
	if (((parameters->InputRegisterData >> 1) & 1) != 1) return 0;
	
	// If DAC code is not confirmed yet, unable to set the Current permission to 1
	if (!parameters->DACcodeConfirmed[0]) return 0;
	
	// If DAC code is not set to 0x8000, unable to set the Current permission to 1
	if (parameters->DACcode[0] != 0x8000) return 0;
	
	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 1, 1);	
}


int controlSinglePermissionOff(int cgwIndex, int deviceId) {
	// No bits check is necessary 
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 1, 0);	
}


int controlSingleInterlockDrop(int cgwIndex, int deviceId) {
	// Check Current and Force permissions are set to 0
	cgwPsMcu_Information_t *parameters;

	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	
	parameters = deviceKit[cgwIndex].parameters[deviceId];
	
	// If the Current permission is ON, unable to reset the Interlock
	if ((parameters->OutputRegisterData >> 1) & 1) return 0;
	// If the Force permission is ON, unable to reset the Interlock 
	if ((parameters->OutputRegisterData >> 2) & 1) return 0;
	
	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 0, 1);	
}


int controlSingleInterlockRestore(int cgwIndex, int deviceId) {
	// No bits check is necessary
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 0, 0);	
}


int controlAllForceOff(int cgwIndex) {
	return controlAllSetSingleOutputBit(cgwIndex, 2, 0);
}


int controlAllPermissionOff(int cgwIndex) {
	return controlAllSetSingleOutputBit(cgwIndex, 1, 0);
}


int controlSingleSetSingleOutputBit(int cgwIndex, int deviceId, int bit, unsigned char value) {
	setOutputRegisters(cgwIndex, deviceId, value << bit, 1 << bit); 
	return 0;
}


int controlAllSetSingleOutputBit(int cgwIndex, int bit, unsigned char value) {
	int deviceIndex, deviceId;
	for (deviceIndex=0; deviceIndex < deviceKit[cgwIndex].registeredNum; deviceIndex++) {
		deviceId = deviceKit[cgwIndex].registeredIDs[deviceIndex];
		setOutputRegisters(cgwIndex, deviceId, value << bit, 1 << bit);
	}
	return 0;
}


int getCanGwStatus(int cgwIndex, int *status) {
	if (cgwConnectionBroken[cgwIndex])
	    (*status) = 0;
	else
		(*status) = 1;
	return 0;
}

// Error state management
int setSingleErrorState(int cgwIndex, int deviceId, char *message) {

	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	
	deviceKit[cgwIndex].error_state[deviceId] = 1;
	strcpy(deviceKit[cgwIndex].last_error_msg[deviceId], message);

	return 0;	
}

int getSingleErrorStateMessage(int cgwIndex, int deviceId, char *buffer) {
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	strcpy(buffer, deviceKit[cgwIndex].last_error_msg[deviceId]);
	
	return 0;	
}

int clearSingleErrorState(int cgwIndex, int deviceId) {
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);
	
	deviceKit[cgwIndex].error_state[deviceId] = 0;

	return 0;	
}

int setAllErrorState(int cgwIndex, char *message) {
	int deviceIndex, deviceId;
	
	for (deviceIndex=0; deviceIndex < deviceKit[cgwIndex].registeredNum; deviceIndex++) {
		deviceId = deviceKit[cgwIndex].registeredIDs[deviceIndex];
		deviceKit[cgwIndex].error_state[deviceId] = 1;
		strcpy(deviceKit[cgwIndex].last_error_msg[deviceId], message);  // clear error message
	}
	
	return 0;	
}

int clearAllErrorState(int cgwIndex) {

	int deviceIndex, deviceId;
	
	for (deviceIndex=0; deviceIndex < deviceKit[cgwIndex].registeredNum; deviceIndex++) {
		deviceId = deviceKit[cgwIndex].registeredIDs[deviceIndex];
		deviceKit[cgwIndex].error_state[deviceId] = 0;
		deviceKit[cgwIndex].last_error_msg[deviceId][0] = 0;  // clear error message
	}
	
	return 0;	
}
