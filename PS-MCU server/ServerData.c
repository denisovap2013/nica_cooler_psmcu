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
#include "Logging.h"

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

// Initialization parameters
int serverDataInitialized = 0;
contactorControlInfo_t contactor_control;;  

//==============================================================================
// Global functions

///////////////////////////////////
///////////////////////////////////
// Queues
///////////////////////////////////
///////////////////////////////////

int _queueRemoveItem(deviceQueue_t * queue, deviceQueueItem_t *elementToDelete) {
    deviceQueueItem_t * item;
	
	// Check that it is in queue
	if (!queue->start) return 0;
	
    item = queue->start;
	while (item) {
		if (item == elementToDelete) {
			
			// Update the pointer to the start of the queue if necessary..
			if (item == queue->start) queue->start = item->next;
			
			// Update the pointer to the end of the queue if necessary
			if (item == queue->end) queue->end = item->prev;
			
			if (item->prev) item->prev->next = item->next;
			if (item->next) item->next->prev = item->prev;
			
			free(item);
			
			queue->length--;
			return 1;
		}
		
        // Go to the next element in the queue
	    item = item->next;	
	}
	return 0;
}


int _idPairsEqual(deviceIdPair_t id_1, deviceIdPair_t id_2) {
    if ((id_1.cgwIndex == id_2.cgwIndex) && (id_1.deviceId == id_2.deviceId)) return 1;
	return 0;
}

void _initQueue(deviceQueue_t * queue) {
    queue->length = 0;
	queue->start = 0;
	queue->end = 0;
}

void _clearQueue(deviceQueue_t * queue) {
	deviceQueueItem_t *item, *next_item;
	
	item = queue->start;

	while(item) {
	    next_item = item->next;
		free(item);
		item = next_item;
	}
	
	queue->length = 0;
	queue->start = 0;
	queue->end = 0;
}


int _queueRemoveItemById(deviceQueue_t * queue, deviceIdPair_t deviceIdPair) {
    deviceQueueItem_t * item, *next_item;
	int deletedElementsNum;
	
	// Check that it is in queue
	if (!queue->start) return 0;
	
    item = queue->start;
	
	deletedElementsNum = 0;
	while (item) {
		next_item = item->next; 

		if (_idPairsEqual(item->deviceIdPair, deviceIdPair)) {
			
			// Update the pointer to the start of the queue if necessary..
			if (item == queue->start) queue->start = item->next;
			
			// Update the pointer to the end of the queue if necessary
			if (item == queue->end) queue->end = item->prev;
			
			if (item->prev) item->prev->next = item->next;
			if (item->next) item->next->prev = item->prev;
			
			deletedElementsNum += 1;
			free(item);
			
		}
		
        // Go to the next element in the queue
	    item = next_item;	
	}
	queue->length -= deletedElementsNum;
	return deletedElementsNum;
}


int queueGet(deviceQueue_t * queue, deviceIdPair_t * deviceIdPair) {
	deviceQueueItem_t *item;

    if (!queue->start) return 0;
    
	item = queue->start;
	(*deviceIdPair) = item->deviceIdPair;
	
	// Could use _queueRemoveItem(), but decided to use more optimized approach (do not need to find an element to delete.
	if (queue->start->next) queue->start->next->prev = 0;
	queue->start = queue->start->next;
	
	if (!queue->start) queue->end = 0;  // No more elements in the queue
	free(item);
	
	queue->length--;
	
	return 1;
}


int queueAdd(deviceQueue_t * queue, deviceIdPair_t deviceIdPair) {
	deviceQueueItem_t *item;
	
	item = malloc(sizeof(deviceQueueItem_t));
	item->deviceIdPair = deviceIdPair;
	item->next = 0;
	item->prev = queue->end;

	queue->length++;

    if (!queue->end) {
	    // Create pointers to the start and end elements in the queue.
		queue->start = item;
		queue->end = item;
	} else {
		// Add element to the end of the queue
	    queue->end->next = item;
		queue->end = item;
	}
	
	return 1;
}


int queueHasElement(deviceQueue_t * queue, deviceIdPair_t deviceIdPair) {
	deviceQueueItem_t *item;   
	
	item = queue->start;
	
	while(item) {
		if (_idPairsEqual(item->deviceIdPair, deviceIdPair)) return 1;
		item = item->next;
	}
	
    return 0;	
}

int queueRemove(deviceQueue_t * queue, deviceIdPair_t deviceIdPair) {
    return _queueRemoveItemById(queue, deviceIdPair);	
}

///////////////////////////////////
///////////////////////////////////
// Device functions    
///////////////////////////////////
///////////////////////////////////

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

	*status = deviceKit[cgwIndex].active[deviceId] | (deviceKit[cgwIndex].error_state[deviceId] << 1);

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
	strcpy(buffer, getDeviceNamePtr(cgwIndex, deviceId));
	return 0;	
}

char * getDeviceNamePtr(int cgwIndex, int deviceId) {
	return ((cgwPsMcu_Information_t*)deviceKit[cgwIndex].parameters[deviceId])->deviceName;
}

cgwPsMcu_Information_t * getDevicePatameters(int cgwIndex, int deviceId) {
    return deviceKit[cgwIndex].parameters[deviceId];	
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
	// DO NOT CALL THIS FUNCTION DIRECTLY.
	// Use the processNextForceOn() to handle the necessary delays between calls.
	
	// Check the Current Permission is set to 0
	cgwPsMcu_Information_t *parameters;
	
	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]);

	parameters = deviceKit[cgwIndex].parameters[deviceId];
	
	// If the Current permission is ON, unable to set the Force permission to 1 (incorrect order) 
	if ((parameters->OutputRegisterData >> 1) & 1) {
		logMessage("[SERVER] The current permission is ON for the device \"%s\". Unable to the the Force permission to 1.",
			getDeviceNamePtr(cgwIndex, deviceId));
		return -1;
	}

	return controlSingleSetSingleOutputBit(cgwIndex, deviceId, 2, 1);	
}


int controlScheduleSingleForceOn(int cgwIndex, int deviceId) {
	cgwPsMcu_Information_t *parameters;
	deviceIdPair_t deviceIdPair;

	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]); 
	
	parameters = deviceKit[cgwIndex].parameters[deviceId]; 
	
    // Check the current Force permission state first, if it is even necessary to process
	// the set the Force permision to 1.
	if ((parameters->OutputRegisterData >> 2) & 1) return 0;
	
	// If the Current permission is ON, unable to set the Force permission to 1 (incorrect order) 
	if ((parameters->OutputRegisterData >> 1) & 1) return 0;

	// Add the specified device to thew queue for switching on contactors.
	deviceIdPair.cgwIndex = cgwIndex;
	deviceIdPair.deviceId = deviceId;
	
	if (queueHasElement(&contactor_control.switch_queue, deviceIdPair)) {
	    logMessage("[SERVER] Tried to switch the contactor on for the device \"%s\" while already wating for it.",
			getDeviceNamePtr(cgwIndex, deviceId));	
	} else {
	    queueAdd(&contactor_control.switch_queue, deviceIdPair);
	}
	
	while(processNextForceOn()) {/*If function returns 1, it means the next element in the queue can be processed immediately.*/}
	
	return 0;
}

int processNextForceOn(void) {
	/*
	  Returns:
	  
	    0 - unable to set
	*/
	
	time_t current_time;  
	deviceIdPair_t deviceIdPair;

	if (!serverDataInitialized) {
		logMessage("[CODE] Tried to proces the contactor swtich-on queue. However, server data is not initinalized.");
		return 0;
	}
	
	time(&current_time);
	
	if (current_time - contactor_control.lastContactorUpdate < contactor_control.lastSetDelay)
		return 0;  // It is still not allowed to switch on the contactor for another device.
	
	if (!contactor_control.switch_queue.length) {
		// Nothing to process
		// (reset the update info)
		contactor_control.lastContactorUpdate = 0;
		contactor_control.lastSetDelay = 0;
		return 0;
	}
	
	if (!queueGet(&contactor_control.switch_queue, &deviceIdPair)) {
	    logMessage("[CODE] Unable to get the device ID from the contactor switch queue.");  
		return 0;	
	}
	
	if (controlSingleForceOn(deviceIdPair.cgwIndex, deviceIdPair.deviceId) < 0) {
	    // Setting the Force permission to 1 did not work.
		// Set the delay to zero to allow immediate switching of the next contactor.
		contactor_control.lastContactorUpdate = 0;
		contactor_control.lastSetDelay = 0;
		return 1;
	}
	
	contactor_control.lastContactorUpdate = current_time;
	contactor_control.lastSetDelay = getDevicePatameters(deviceIdPair.cgwIndex, deviceIdPair.deviceId)->contactorDelay;
	
	if (contactor_control.lastSetDelay == 0) {
		logMessage("[SERVER] Delay is not set fo this device's contactor. Proceeding to the next element in the queue.");
		contactor_control.lastContactorUpdate = 0;
	    contactor_control.lastSetDelay = 0;
		return 1;
	}
	
	logMessage(
		"[SERVER] Waiting for %lf.2 seconds before the next contactor of other devices can be turned on.",
		contactor_control.lastSetDelay);	
	
	return 0;
}


int controlSingleForceOff(int cgwIndex, int deviceId) {
	// No bits check is necessary  
	
	cgwPsMcu_Information_t *parameters;
	deviceIdPair_t deviceIdPair; 

	CHECK_DEVICE_ID(deviceId, deviceKit[cgwIndex]); 
	
	parameters = deviceKit[cgwIndex].parameters[deviceId]; 
	
	// Remove the device from the queue for switching the contactor ON, if it was in the queue.
	deviceIdPair.cgwIndex = cgwIndex;
	deviceIdPair.deviceId = deviceId;
	queueRemove(&contactor_control.switch_queue, deviceIdPair);

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

// Initialization
void InitServerData(void) {
	if (serverDataInitialized) {
		logMessage("[CODE] attempted to initialize the server data twice.");
		return;
	}

	contactor_control.lastContactorUpdate = 0;
	contactor_control.lastSetDelay = 0;
	
	// Initializing a queue for the 
	_initQueue(&contactor_control.switch_queue);

    serverDataInitialized = 1;
	logMessage("[SERVER] Server data and states are initialized.");
}


void ReleaseServerData(void) {
	if (!serverDataInitialized) return;
	
	_clearQueue(&contactor_control.switch_queue);

	serverDataInitialized = 0;
	logMessage("[SERVER] Server data and states are freed.");	
}

