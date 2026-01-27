//==============================================================================
//
// Title:       ServerData.h
// Purpose:     A short description of the interface.
//
// Created on:  15.12.2021 at 13:22:13 by Vasya.
// Copyright:   Laboratory of anomalous materials. All Rights Reserved.
//
//==============================================================================

#ifndef __ServerData_H__
#define __ServerData_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "cvidef.h"

#include "cangw.h"   
		
#include "TCP_Connection.h" 
#include "CGW_Devices.h"
#include "ServerConfigData.h" 
		
//==============================================================================
// Constants

//==============================================================================
// Types
struct deviceQueueItem_t;
typedef struct deviceQueueItem_t deviceQueueItem_t;

typedef struct deviceIdPair_t {
    int cgwIndex;
	int deviceId;	
} deviceIdPair_t;

struct deviceQueueItem_t {
    deviceQueueItem_t *prev;
	deviceQueueItem_t *next;
	deviceIdPair_t deviceIdPair;
};

typedef struct deviceQueue_t {
    deviceQueueItem_t *start;
	deviceQueueItem_t *end;
	int length;
} deviceQueue_t;

int queueGet(deviceQueue_t * queue, deviceIdPair_t * deviceIdPair);
int queueAdd(deviceQueue_t * queue, deviceIdPair_t deviceIdPair);
int queueHasElement(deviceQueue_t * queue, deviceIdPair_t deviceIdPair);


typedef struct contactorControlInfo_t {
    time_t lastContactorUpdate;
	double lastSetDelay;
	deviceQueue_t switch_queue;
} contactorControlInfo_t;

//==============================================================================
// External variables
extern cgw_devices_t deviceKit[CFG_CGW_MAX_NUM];
extern tcpConnection_ServerInterface_t tcpSI;

//==============================================================================
// Global functions
int deviceIndexToDeviceId(int globalDeviceIndex, int *cgwIndex, int *deviceId);

int getDeviceFullInfo(int cgwIndex, int deviceId, char *formattedInfo);
int getDeviceStatus(int cgwIndex, int deviceId, unsigned int *status);

int setDacVoltage(int cgwIndex, int deviceId, int channel, double voltage, int raw, int slow);
int getAdcVoltage(int cgwIndex, int deviceId, int channel, double *result, int raw);
int getAllRegisters(int cgwIndex, int deviceId, unsigned char *inputRegisters, unsigned char *outputRegisters);
int setOutputRegisters(int cgwIndex, int deviceId, unsigned char registers, unsigned char mask);

int resetSingleAdcMeasurements(int cgwIndex, int deviceId); 
int resetAllAdcMeasurements(int cgwIndex);
int stopSingleAdcMeasurements(int cgwIndex, int deviceId); 
int stopAllAdcMeasurements(int cgwIndex);

int getAdcChannelName(int cgwIndex, int deviceId, int channel, char *buffer);
int getDacChannelName(int cgwIndex, int deviceId, int channel, char *buffer);
int getInputRegisterName(int cgwIndex, int deviceId, int registerIndex, char *buffer);
int getOutputRegisterName(int cgwIndex, int deviceId, int registerIndex, char *buffer);
int getDeviceName(int cgwIndex, int deviceId, char *buffer);
char * getDeviceNamePtr(int cgwIndex, int deviceId);
cgwPsMcu_Information_t * getDevicePatameters(int cgwIndex, int deviceId);

int setAllDacToZero(int cgwIndex);

int controlSingleForceOn(int cgwIndex, int deviceId);
int controlScheduleSingleForceOn(int cgwIndex, int deviceId);
int processNextForceOn(void);

int controlSingleForceOff(int cgwIndex, int deviceId);
int controlSinglePermissionOn(int cgwIndex, int deviceId);
int controlSinglePermissionOff(int cgwIndex, int deviceId);
int controlSingleInterlockDrop(int cgwIndex, int deviceId);
int controlSingleInterlockRestore(int cgwIndex, int deviceId);

int controlAllForceOff(int cgwIndex);
int controlAllPermissionOff(int cgwIndex);

int getCanGwStatus(int cgwIndex, int *status);

// Error state management
int setSingleErrorState(int cgwIndex, int deviceId, char *message);
int getSingleErrorStateMessage(int cgwIndex, int deviceId, char *buffer);
int clearSingleErrorState(int cgwIndex, int deviceId);

int setAllErrorState(int cgwIndex, char *message);
int clearAllErrorState(int cgwIndex);

// Initialization
void InitServerData(void);
void ReleaseServerData(void);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ServerData_H__ */
