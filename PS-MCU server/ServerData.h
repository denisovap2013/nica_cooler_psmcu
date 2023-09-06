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

int setAllDacToZero(int cgwIndex);

int controlSingleForceOn(int cgwIndex, int deviceId);
int controlSingleForceOff(int cgwIndex, int deviceId);
int controlSinglePermissionOn(int cgwIndex, int deviceId);
int controlSinglePermissionOff(int cgwIndex, int deviceId);
int controlSingleInterlockDrop(int cgwIndex, int deviceId);
int controlSingleInterlockRestore(int cgwIndex, int deviceId);

int controlAllForceOff(int cgwIndex);
int controlAllPermissionOff(int cgwIndex);

int getCanGwStatus(int cgwIndex, int *status);


#ifdef __cplusplus
    }
#endif

#endif  /* ndef __ServerData_H__ */
